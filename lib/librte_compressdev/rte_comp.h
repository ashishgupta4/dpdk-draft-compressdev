/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2017-2018 Intel Corporation
 */

#ifndef _RTE_COMP_H_
#define _RTE_COMP_H_

/**
 * @file rte_comp.h
 *
 * RTE definitions for Data Compression Service
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <rte_mempool.h>


/** Status of comp operation */
enum rte_comp_op_status {
	RTE_COMP_OP_STATUS_SUCCESS = 0,
	/**< Operation completed successfully */
	RTE_COMP_OP_STATUS_NOT_PROCESSED,
	/**< Operation has not yet been processed by the device */
	RTE_COMP_OP_STATUS_INVALID_SESSION,
	/**< Operation failed due to invalid session arguments */
	RTE_COMP_OP_STATUS_INVALID_ARGS,
	/**< Operation failed due to invalid arguments in request */
	RTE_COMP_OP_STATUS_ERROR,
	/**< Error handling operation */
	RTE_COMP_OP_STATUS_INVALID_STATE,
	/**< Operation is invoked in invalid state */
	RTE_COMP_OP_STATUS_OUT_OF_SPACE,
	/**< Output buffer ran out of space before operation completed */
};


/** Compression Algorithms */
enum rte_comp_algorithm {
	RTE_COMP_NULL = 0,
	/**< No compression.
	 * Pass-through, data is copied unchanged from source buffer to
	 * destination buffer.
	 */
	RTE_COMP_DEFLATE,
	/**< DEFLATE compression algorithm
	 * https://tools.ietf.org/html/rfc1951
	 */
	RTE_COMP_LZS,
	/**< LZS compression algorithm
	 * https://tools.ietf.org/html/rfc2395
	 */
	RTE_COMP_ALGO_LIST_END
};

/**< Compression Level.
 * The number is interpreted by each PMD differently. However, lower numbers
 * give fastest compression, at the expense of compression ratio while
 * higher numbers may give better compression ratios but are likely slower.
 */
#define	RTE_COMP_LEVEL_PMD_DEFAULT	(-1)
/** Use PMD Default */
#define	RTE_COMP_LEVEL_NONE		(0)
/** Output uncompressed blocks if supported by the specified algorithm */
#define RTE_COMP_LEVEL_MIN		(1)
/** Use minimum compression level supported by the PMD */
#define RTE_COMP_LEVEL_MAX		(9)
/** Use maximum compression level supported by the PMD */

/** Compression checksum types */
enum rte_comp_checksum_type {
	RTE_COMP_NONE,
	/**< No checksum generated */
	RTE_COMP_CRC32,
	/**< Generates a CRC32 checksum, as used by gzip */
	RTE_COMP_ADLER32,
	/**< Generates an Adler-32 checksum, as used by zlib */
	RTE_COMP_CRC32_ADLER32,
	/**< Generates both Adler-32 and CRC32 checksums, concatenated.
	 * CRC32 is in the lower 32bits, Adler-32 in the upper 32 bits.
	 */
};


/** Compression Huffman Type - used by DEFLATE algorithm */
enum rte_comp_huffman {
	RTE_COMP_DEFAULT,
	/**< PMD may choose which Huffman codes to use */
	RTE_COMP_FIXED,
	/**< Use Fixed Huffman codes */
	RTE_COMP_DYNAMIC,
	/**< Use Dynamic Huffman codes */
};

enum rte_comp_flush_flag {
	RTE_COMP_FLUSH_NONE,
	/**< Data is not flushed. Output may remain in the compressor and be
	 * processed during a following op. It may not be possible to decompress
	 * output until a later op with some other flush flag has been sent.
	 */
	RTE_COMP_FLUSH_SYNC,
	/**< All data should be flushed to output buffer. Output data can be
	 * decompressed. However state and history is not cleared, so future
	 * operations may use history from this operation.
	 */
	RTE_COMP_FLUSH_FULL,
	/**< All data should be flushed to output buffer. Output data can be
	 * decompressed. State and history data is cleared, so future
	 * ops will be independent of ops processed before this.
	 */
	RTE_COMP_FLUSH_FINAL
	/**< Same as RTE_COMP_FLUSH_FULL but if op.algo is RTE_COMP_DEFLATE
	 * then bfinal bit is set in the last block.
	 */
};

/** Compression transform types */
enum rte_comp_xform_type {
	RTE_COMP_COMPRESS,
	/**< Compression service - compress */
	RTE_COMP_DECOMPRESS,
	/**< Compression service - decompress */
};

enum rte_comp_op_type {
	RTE_COMP_OP_STATELESS,
	/**< All data to be processed is submitted in the op, no state or history
	 * from previous ops is used and none will be stored for future ops.
	 * Flush flag must be set to either FLUSH_FULL or FLUSH_FINAL.
	 */
	RTE_COMP_OP_STATEFUL
	/**< There may be more data to be processed after this op, it's part of a
	 * stream of data. State and history from previous ops can be used
	 * and resulting state and history can be stored for future ops,
	 * depending on flush flag.
	 */
};


/** Parameters specific to the deflate algorithm */
struct rte_comp_deflate_params {
	enum rte_comp_huffman huffman;
	/**< Compression huffman encoding type */
};

/** Session Setup Data for compression */
struct rte_comp_compress_xform {
	enum rte_comp_algorithm algo;
	/**< Algorithm to use for compress operation */
	union {
		struct rte_comp_deflate_params deflate;
		/**< Parameters specific to the deflate algorithm */
	}; /**< Algorithm specific parameters */
	int level;
	/**< Compression level */
	uint8_t window_size;
	/**< Base two log value of the sliding window size which was
	* used to generate compressed data. If window size can't be
	* supported by the PMD then session setup should fail.
	*/
	enum rte_comp_checksum_type chksum;
	/**< Type of checksum to generate on the uncompressed data */
};

/**
 * Session Setup Data for decompression.
 */
struct rte_comp_decompress_xform {
	enum rte_comp_algorithm algo;
	/**< Algorithm to use for decompression */
	enum rte_comp_checksum_type chksum;
	/**< Type of checksum to generate on the decompressed data */
	uint8_t window_size;
	/**< Base two log value of the sliding window size which was
	* used to generate compressed data. If window size can't be
	* supported by the PMD then session setup should fail.
	*/
};

/**
 * Compression transform structure.
 *
 * This is used to specify the compression transforms required.
 * Each transform structure can hold a single transform, the type field is
 * used to specify which transform is contained within the union.
 * There are no chain cases currently supported, just single xforms of
 *  - compress-only
 *  - decompress-only
 *
 */
struct rte_comp_xform {
	struct rte_comp_xform *next;
	/**< next xform in chain */
	enum rte_comp_xform_type type;
	/**< xform type */
	union {
		struct rte_comp_compress_xform compress;
		/**< xform for compress operation */
		struct rte_comp_decompress_xform decompress;
		/**< decompress xform */
	};
};


struct rte_comp_session;
/**
 * Compression Operation.
 *
 * This structure contains data relating to performing a compression
 * operation on the referenced mbuf data buffers.
 *
 * Comp operations are enqueued and dequeued in comp PMDs using the
 * rte_compressdev_enqueue_burst() / rte_compressdev_dequeue_burst() APIs
 */
struct rte_comp_op {

	enum rte_comp_op_type op_type;
	void *stream_private;
	/* Handle for an initialised stream, which holds state and history data.
	 * rte_comp_stream_create() must be called for all data streams whether
	 * op_type is STATEFUL or STATELESS and the resulting stream attached
	 * to the one or more operations associated with the data stream.
	 */
	struct rte_comp_session *session;
	/**< Handle for the initialised session context */
	struct rte_mempool *mempool;
	/**< Pool from which operation is allocated */
	rte_iova_t phys_addr;
	/**< Physical address of this operation */
	struct rte_mbuf *m_src;
	/**< source mbuf
	 * The total size of the input buffer(s) can be retrieved using
	 * rte_pktmbuf_data_len(m_src)
	 */
	struct rte_mbuf *m_dst;
	/**< destination mbuf
	 * The total size of the output buffer(s) can be retrieved using
	 * rte_pktmbuf_data_len(m_dst)
	 */

	struct {
		uint32_t offset;
		/**< Starting point for compression or decompression,
		 * specified as number of bytes from start of packet in
		 * source buffer.
		 * Starting point for checksum generation in compress direction.
		 */
		uint32_t length;
		/**< The length, in bytes, of the data in source buffer
		 * to be compressed or decompressed.
		 * Also the length of the data over which the checksum
		 * should be generated in compress direction
		 */
	} src;
	struct {
		uint32_t offset;
		/**< Starting point for writing output data, specified as
		 * number of bytes from start of packet in dest
		 * buffer. Starting point for checksum generation in
		 * decompress direction.
		 */
	} dst;
	enum rte_comp_flush_flag flush_flag;
	/**< Defines flush characteristics for the output data.
	 * Only applicable in compress direction
	 */
	uint64_t input_chksum;
	/**< An input checksum can be provided to generate a
	 * cumulative checksum across sequential blocks in a STATELESS stream.
	 * Checksum type is as specified in xform chksum_type
	 */
	uint64_t output_chksum;
	/**< If a checksum is generated it will be written in here.
	 * Checksum type is as specified in xform chksum_type.
	 */
	uint32_t consumed;
	/**< The number of bytes from the source buffer
	 * which were compressed/decompressed.
	 */
	uint32_t produced;
	/**< The number of bytes written to the destination buffer
	 * which were compressed/decompressed.
	 */
	uint64_t debug_status;
	/**<
	 * Status of the operation is returned in the status param.
	 * This field allows the PMD to pass back extra
	 * pmd-specific debug information. Value is not defined on the API.
	 */
	uint8_t status;
	/**<
	 * Operation status - use values from enum rte_comp_status.
	 * This is reset to
	 * RTE_COMP_OP_STATUS_NOT_PROCESSED on allocation from mempool and
	 * will be set to RTE_COMP_OP_STATUS_SUCCESS after operation
	 * is successfully processed by a PMD
	 */
};


/**
 * Reset the fields of an operation to their default values.
 *
 * @note The private data associated with the operation is not zeroed.
 *
 * @param op
 *   The operation to be reset
 */
static inline void
__rte_comp_op_reset(struct rte_comp_op *op)
{
	struct rte_mempool *tmp_mp = op->mempool;
	phys_addr_t tmp_phys_addr = op->phys_addr;

	memset(op, 0, sizeof(struct rte_comp_op));
	op->status = RTE_COMP_OP_STATUS_NOT_PROCESSED;
	op->phys_addr = tmp_phys_addr;
	op->mempool = tmp_mp;
}

/**
 * Private data structure belonging to an operation pool.
 */
struct rte_comp_op_pool_private {
	uint16_t user_size;
	/**< Size of private user data with each operation. */
};


/**
 * Returns the size of private user data allocated with each object in
 * the mempool
 *
 * @param mempool
 *   Mempool for operations
 * @return
 *   user data size
 */
static inline uint16_t
__rte_comp_op_get_user_data_size(struct rte_mempool *mempool)
{
	struct rte_comp_op_pool_private *priv =
	    (struct rte_comp_op_pool_private *)rte_mempool_get_priv(mempool);

	return priv->user_size;
}


/**
 * Creates an operation pool
 *
 * @param name
 *   Compress pool name
 * @param nb_elts
 *   Number of elements in pool
 * @param cache_size
 *   Number of elements to cache on lcore, see
 *   *rte_mempool_create* for further details about cache size
 * @param user_size
 *   Size of private data to allocate for user with each operation
 * @param socket_id
 *   Socket to identifier allocate memory on
 * @return
 *  - On success pointer to mempool
 *  - On failure NULL
 */
struct rte_mempool *
rte_comp_op_pool_create(const char *name,
		unsigned int nb_elts, unsigned int cache_size,
		uint16_t user_size, int socket_id);

/**
 * Bulk allocate raw element from mempool and return as comp operations
 *
 * @param mempool
 *   Compress operation mempool
 * @param ops
 *   Array to place allocated operations
 * @param nb_ops
 *   Number of operations to allocate
 * @return
 * - On success returns  number of ops allocated
 */
static inline int
__rte_comp_op_raw_bulk_alloc(struct rte_mempool *mempool,
		struct rte_comp_op **ops, uint16_t nb_ops)
{
	if (rte_mempool_get_bulk(mempool, (void **)ops, nb_ops) == 0)
		return nb_ops;

	return 0;
}

/**
 * Allocate an operation from a mempool with default parameters set
 *
 * @param mempool
 *   Compress operation mempool
 *
 * @return
 * - On success returns a valid rte_comp_op structure
 * - On failure returns NULL
 */
static inline struct rte_comp_op *
rte_comp_op_alloc(struct rte_mempool *mempool)
{
	struct rte_comp_op *op = NULL;
	int retval;

	retval = __rte_comp_op_raw_bulk_alloc(mempool, &op, 1);
	if (unlikely(retval != 1))
		return NULL;

	__rte_comp_op_reset(op);

	return op;
}


/**
 * Bulk allocate operations from a mempool with default parameters set
 *
 * @param mempool
 *   Compress operation mempool
 * @param ops
 *   Array to place allocated operations
 * @param nb_ops
 *   Number of operations to allocate
 * @return
 * - nb_ops if the number of operations requested were allocated.
 * - 0 if the requested number of ops are not available.
 *   None are allocated in this case.
 */
static inline unsigned
rte_comp_op_bulk_alloc(struct rte_mempool *mempool,
		struct rte_comp_op **ops, uint16_t nb_ops)
{
	int i;

	if (unlikely(__rte_comp_op_raw_bulk_alloc(mempool, ops, nb_ops)
			!= nb_ops))
		return 0;

	for (i = 0; i < nb_ops; i++)
		__rte_comp_op_reset(ops[i]);

	return nb_ops;
}



/**
 * Returns a pointer to the private user data of an operation if
 * that operation has enough capacity for requested size.
 *
 * @param op
 *   Compress operation
 * @param size
 *   Size of space requested in private data
 * @return
 * - if sufficient space available returns pointer to start of user data
 * - if insufficient space returns NULL
 */
static inline void *
__rte_comp_op_get_user_data(struct rte_comp_op *op, uint32_t size)
{
	uint32_t user_size;

	if (likely(op->mempool != NULL)) {
		user_size = __rte_comp_op_get_user_data_size(op->mempool);

		if (likely(user_size >= size))
			return (void *)(op + 1);

	}

	return NULL;
}

/**
 * free operation structure
 * If operation has been allocate from a rte_mempool, then the operation will
 * be returned to the mempool.
 *
 * @param op
 *   Compress operation
 */
static inline void
rte_comp_op_free(struct rte_comp_op *op)
{
	if (op != NULL && op->mempool != NULL)
		rte_mempool_put(op->mempool, op);
}

#ifdef __cplusplus
}
#endif

#endif /* _RTE_COMP_H_ */
