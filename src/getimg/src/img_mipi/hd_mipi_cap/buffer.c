/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "buffer_internal.h"
#include "buffer.h"

static atomic_size_t max_alloc_size = INT_MAX; // NOLINT

#define MIN(a,b) ((a) > (b) ? (b) : (a))

void *sx_av_malloc(size_t size)
{
    void *ptr = NULL;

    if (size > atomic_load_explicit(&max_alloc_size, memory_order_relaxed))
        return NULL;

#if HAVE_POSIX_MEMALIGN
    if (size) //OS X on SDK 10.6 has a broken posix_memalign implementation
    if (posix_memalign(&ptr, ALIGN, size))
        ptr = NULL;
#elif HAVE_ALIGNED_MALLOC
    ptr = _aligned_malloc(size, ALIGN);
#elif HAVE_MEMALIGN
#ifndef __DJGPP__
    ptr = memalign(ALIGN, size);
#else
    ptr = memalign(size, ALIGN);
#endif
    /* Why 64?
     * Indeed, we should align it:
     *   on  4 for 386
     *   on 16 for 486
     *   on 32 for 586, PPro - K6-III
     *   on 64 for K7 (maybe for P3 too).
     * Because L1 and L2 caches are aligned on those values.
     * But I don't want to code such logic here!
     */
    /* Why 32?
     * For AVX ASM. SSE / NEON needs only 16.
     * Why not larger? Because I did not see a difference in benchmarks ...
     */
    /* benchmarks with P3
     * memalign(64) + 1          3071, 3051, 3032
     * memalign(64) + 2          3051, 3032, 3041
     * memalign(64) + 4          2911, 2896, 2915
     * memalign(64) + 8          2545, 2554, 2550
     * memalign(64) + 16         2543, 2572, 2563
     * memalign(64) + 32         2546, 2545, 2571
     * memalign(64) + 64         2570, 2533, 2558
     *
     * BTW, malloc seems to do 8-byte alignment by default here.
     */
#else
    ptr = malloc(size);
#endif
    if(!ptr && !size) {
        size = 1;
        ptr= sx_av_malloc(1);
    }
#if CONFIG_MEMORY_POISONING
    if (ptr)
        memset(ptr, FF_MEMORY_POISON, size);
#endif
    return ptr;
}

void *sx_av_realloc(void *ptr, size_t size)
{
    void *ret = NULL;
    if (size > atomic_load_explicit(&max_alloc_size, memory_order_relaxed))
        return NULL;

#if HAVE_ALIGNED_MALLOC
    ret = _aligned_realloc(ptr, size + !size, ALIGN);
#else
    ret = realloc(ptr, size + !size);
#endif
#if CONFIG_MEMORY_POISONING
    if (ret && !ptr)
        memset(ret, FF_MEMORY_POISON, size);
#endif
    return ret;
}

void sx_av_free(void *ptr)
{
#if HAVE_ALIGNED_MALLOC
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

void sx_av_freep(void *arg)
{
    void *val = NULL;

    memcpy(&val, arg, sizeof(val));
    memcpy(arg, &(void *){ NULL }, sizeof(val));
    sx_av_free(val);
}

void *sx_av_mallocz(size_t size)
{
    void *ptr = sx_av_malloc(size);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}


static AVBufferRef *buffer_create(AVBuffer *buf, uint8_t *data, size_t size,
                                  void (*free)(void *opaque, uint8_t *data),
                                  void *opaque, int flags)
{
    AVBufferRef *ref = NULL;

    buf->data     = data;
    buf->size     = size;
    buf->free     = free ? free : sx_av_buffer_default_free;
    buf->opaque   = opaque;

    atomic_init(&buf->refcount, 1);

    buf->flags = flags;

    ref = sx_av_mallocz(sizeof(*ref));
    if (!ref)
        return NULL;

    ref->buffer = buf;
    ref->data   = data;
    ref->size   = size;

    return ref;
}

AVBufferRef *sx_av_buffer_create(uint8_t *data, size_t size,
                              void (*free)(void *opaque, uint8_t *data),
                              void *opaque, int flags)
{
    AVBufferRef *ret = NULL;
    AVBuffer *buf = sx_av_mallocz(sizeof(*buf));
    if (!buf)
        return NULL;

    ret = buffer_create(buf, data, size, free, opaque, flags);
    if (!ret) {
        sx_av_free(buf);
        return NULL;
    }
    return ret;
}

void sx_av_buffer_default_free(void *opaque, uint8_t *data)
{
    sx_av_free(data);
}

AVBufferRef *sx_av_buffer_alloc(size_t size)
{
    AVBufferRef *ret = NULL;
    uint8_t    *data = NULL;

    data = sx_av_malloc(size);
    if (!data)
        return NULL;

    ret = sx_av_buffer_create(data, size, sx_av_buffer_default_free, NULL, 0);
    if (!ret)
        sx_av_freep(&data);

    return ret;
}

AVBufferRef *sx_av_buffer_allocz(size_t size)
{
    AVBufferRef *ret = sx_av_buffer_alloc(size);
    if (!ret)
        return NULL;

    memset(ret->data, 0, size);
    return ret;
}

AVBufferRef *sx_av_buffer_ref(const AVBufferRef *buf)
{
    AVBufferRef *ret = sx_av_mallocz(sizeof(*ret));

    if (!ret)
        return NULL;

    *ret = *buf;

    atomic_fetch_add_explicit(&buf->buffer->refcount, 1, memory_order_relaxed);

    return ret;
}

static void buffer_replace(AVBufferRef **dst, AVBufferRef **src)
{
    AVBuffer *b = NULL;

    b = (*dst)->buffer;

    if (src) {
        **dst = **src;
        sx_av_freep(src);
    } else
        sx_av_freep(dst);  // *dst 被置为NULL

    if (atomic_fetch_sub_explicit(&b->refcount, 1, memory_order_acq_rel) == 1) {
        /* b->free below might already free the structure containing *b,
         * so we have to read the flag now to avoid use-after-free. */
        int free_avbuffer = !(b->flags_internal & BUFFER_FLAG_NO_FREE);
        b->free(b->opaque, b->data);
        if (free_avbuffer)
            sx_av_free(b);
    }
}

void sx_av_buffer_unref(AVBufferRef **buf)
{
    if (!buf || !*buf)
        return;

    buffer_replace(buf, NULL);
}

int sx_av_buffer_is_writable(const AVBufferRef *buf)
{
    if (buf->buffer->flags & sx_av_BUFFER_FLAG_READONLY)
        return 0;

    return atomic_load(&buf->buffer->refcount) == 1;
}

void *sx_av_buffer_get_opaque(const AVBufferRef *buf)
{
    return buf->buffer->opaque;
}

int sx_av_buffer_get_ref_count(const AVBufferRef *buf)
{
    return atomic_load(&buf->buffer->refcount);
}

int sx_av_buffer_make_writable(AVBufferRef **pbuf)
{
    AVBufferRef *newbuf = NULL, *buf = *pbuf;

    if (sx_av_buffer_is_writable(buf))
        return 0;

    newbuf = sx_av_buffer_alloc(buf->size);
    if (!newbuf)
        return -1;

    memcpy(newbuf->data, buf->data, buf->size);

    buffer_replace(pbuf, &newbuf);

    return 0;
}

int sx_av_buffer_realloc(AVBufferRef **pbuf, size_t size)
{
    AVBufferRef *buf = *pbuf;
    uint8_t *tmp = NULL;
    int ret = 0;

    if (!buf) {
        /* allocate a new buffer with sx_av_realloc(), so it will be reallocatable
         * later */
        uint8_t *data = sx_av_realloc(NULL, size);
        if (!data)
            return -1;

        buf = sx_av_buffer_create(data, size, sx_av_buffer_default_free, NULL, 0);
        if (!buf) {
            sx_av_freep(&data);
            return -1;
        }

        buf->buffer->flags_internal |= BUFFER_FLAG_REALLOCATABLE;
        *pbuf = buf;

        return 0;
    } else if (buf->size == size)
        return 0;

    if (!(buf->buffer->flags_internal & BUFFER_FLAG_REALLOCATABLE) ||
        !sx_av_buffer_is_writable(buf) || buf->data != buf->buffer->data) {
        /* cannot realloc, allocate a new reallocable buffer and copy data */
        AVBufferRef *new = NULL;

        ret = sx_av_buffer_realloc(&new, size);
        if (ret < 0)
            return ret;

        memcpy(new->data, buf->data, MIN(size, buf->size));

        buffer_replace(pbuf, &new);
        return 0;
    }

    tmp = sx_av_realloc(buf->buffer->data, size);
    if (!tmp)
        return -1;

    buf->buffer->data = buf->data = tmp;
    buf->buffer->size = buf->size = size;
    return 0;
}

int sx_av_buffer_replace(AVBufferRef **pdst, const AVBufferRef *src)
{
    AVBufferRef *dst = *pdst;
    AVBufferRef *tmp = NULL;

    if (!src) {
        sx_av_buffer_unref(pdst);
        return 0;
    }

    if (dst && dst->buffer == src->buffer) {
        /* make sure the data pointers match */
        dst->data = src->data;
        dst->size = src->size;
        return 0;
    }

    tmp = sx_av_buffer_ref(src);
    if (!tmp)
        return -1;

    sx_av_buffer_unref(pdst);
    *pdst = tmp;
    return 0;
}

AVBufferPool *sx_av_buffer_pool_init2(size_t size, void *opaque,
                                   AVBufferRef* (*alloc)(void *opaque, size_t size),
                                   void (*pool_free)(void *opaque))
{
    AVBufferPool *pool = sx_av_mallocz(sizeof(*pool));
    if (!pool)
        return NULL;

    pthread_mutex_init(&pool->mutex, NULL);

    pool->size      = size;
    pool->opaque    = opaque;
    pool->alloc2    = alloc;
    pool->alloc     = sx_av_buffer_alloc; // fallback
    pool->pool_free = pool_free;

    atomic_init(&pool->refcount, 1);

    return pool;
}

AVBufferPool *sx_av_buffer_pool_init(size_t size, AVBufferRef* (*alloc)(size_t size))
{
    AVBufferPool *pool = sx_av_mallocz(sizeof(*pool));
    if (!pool)
        return NULL;

    pthread_mutex_init(&pool->mutex, NULL);

    pool->size     = size;
    pool->alloc    = alloc ? alloc : sx_av_buffer_alloc;

    atomic_init(&pool->refcount, 1);

    return pool;
}

static void buffer_pool_flush(AVBufferPool *pool)
{
    while (pool->pool) {
        BufferPoolEntry *buf = pool->pool;
        pool->pool = buf->next;

        buf->free(buf->opaque, buf->data);
        sx_av_freep(&buf);
    }
}

/*
 * This function gets called when the pool has been uninited and
 * all the buffers returned to it.
 */
static void buffer_pool_free(AVBufferPool *pool)
{
    buffer_pool_flush(pool);
    pthread_mutex_destroy(&pool->mutex);

    if (pool->pool_free)
        pool->pool_free(pool->opaque);

    sx_av_freep(&pool);
}

void sx_av_buffer_pool_uninit(AVBufferPool **ppool)
{
    AVBufferPool *pool = NULL;

    if (!ppool || !*ppool)
        return;
    pool   = *ppool;
    *ppool = NULL;

    pthread_mutex_lock(&pool->mutex);
    buffer_pool_flush(pool);
    pthread_mutex_unlock(&pool->mutex);

    if (atomic_fetch_sub_explicit(&pool->refcount, 1, memory_order_acq_rel) == 1)
        buffer_pool_free(pool);
}

static void pool_release_buffer(void *opaque, uint8_t *data)
{
    BufferPoolEntry *buf = opaque;
    AVBufferPool *pool = buf->pool;

    pthread_mutex_lock(&pool->mutex);
    buf->next = pool->pool;
    pool->pool = buf;
    pthread_mutex_unlock(&pool->mutex);

    if (atomic_fetch_sub_explicit(&pool->refcount, 1, memory_order_acq_rel) == 1)
        buffer_pool_free(pool);
}

/* allocate a new buffer and override its free() callback so that
 * it is returned to the pool on free */
static AVBufferRef *pool_alloc_buffer(AVBufferPool *pool)
{
    BufferPoolEntry *buf = NULL;
    AVBufferRef     *ret = NULL;

    if(!(pool->alloc || pool->alloc2)) {
        abort();
    }

    ret = pool->alloc2 ? pool->alloc2(pool->opaque, pool->size) :
                         pool->alloc(pool->size);
    if (!ret)
        return NULL;

    buf = sx_av_mallocz(sizeof(*buf));
    if (!buf) {
        sx_av_buffer_unref(&ret);
        return NULL;
    }

    buf->data   = ret->buffer->data;
    buf->opaque = ret->buffer->opaque;
    buf->free   = ret->buffer->free;
    buf->pool   = pool;

    ret->buffer->opaque = buf;
    ret->buffer->free   = pool_release_buffer;

    return ret;
}

AVBufferRef *sx_av_buffer_pool_get(AVBufferPool *pool)
{
    AVBufferRef *ret = NULL;
    BufferPoolEntry *buf = NULL;

    pthread_mutex_lock(&pool->mutex);
    buf = pool->pool;
    if (buf) {
        memset(&buf->buffer, 0, sizeof(buf->buffer));
        ret = buffer_create(&buf->buffer, buf->data, pool->size,
                            pool_release_buffer, buf, 0);
        if (ret) {
            pool->pool = buf->next;
            buf->next = NULL;
            buf->buffer.flags_internal |= BUFFER_FLAG_NO_FREE;
        }
    } else {
        ret = pool_alloc_buffer(pool);
    }
    pthread_mutex_unlock(&pool->mutex);

    if (ret)
        atomic_fetch_add_explicit(&pool->refcount, 1, memory_order_relaxed);

    return ret;
}

void *sx_av_buffer_pool_buffer_get_opaque(const AVBufferRef *ref)
{
    BufferPoolEntry *buf = ref->buffer->opaque;
    if (!buf) {
        abort();
    }
    return buf->opaque;
}
