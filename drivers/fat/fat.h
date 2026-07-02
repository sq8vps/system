#ifndef FAT_FAT_H_
#define FAT_FAT_H_

#include "defines.h"
#include <stdbool.h>

struct IoRp;
struct FatVolume;



/**
 * @brief Get next cluster number
 * @param *vol FAT volume structure
 * @param currentCluster Current cluster
 * @return Next cluster
 */
uint32_t FatGetNextCluster(struct FatVolume *vol, uint32_t currentCluster);

/**
 * @brief Get number of consecutive clusters
 * @param *vol FAT volume structure
 * @param cluster Starting cluster
 * @return Number of contiguous/consecutive clusters
 */
uint32_t FatGetConsecutiveClusterCount(struct FatVolume *vol, uint32_t cluster);

/**
 * @brief Get last cluster of the cluster chain
 * @param *vol FAT volume structure
 * @param cluster Starting cluster
 * @return Last cluster in the chain
 */
uint32_t FatGetLastCluster(struct FatVolume *vol, uint32_t cluster);

/**
 * @brief Reserve cluster
 * @param *vol FAT volume structure
 * @param cluster Starting cluster (0 to start a new chain)
 * @param count Number of clusters to reserve
 * @return First cluster, 0 on failure
 */
uint32_t FatReserveClusters(struct FatVolume *vol, uint32_t cluster, uint32_t count);

/**
 * @brief Write next cluster (or free or EOF) value
 * @param *vol FAT volume structure
 * @param currentCluster Current cluster number
 * @param nextCluster Next cluster number, free value or EOF value
 */
void FatWriteNextCluster(struct FatVolume *vol, uint32_t currentCluster, uint32_t nextCluster);

/**
 * @brief Free reserved cluster
 * @param *vol FAT volume structure
 * @param cluster Starting cluster to be marked as EOF
 */
void FatFreeClusters(struct FatVolume *vol, uint32_t cluster);

/**
 * @brief Write FATs to disk if changed
 * @param *vol FAT volume structure
 */
void FatUpdateOnDisk(struct FatVolume *vol);

/**
 * @brief Check if given cluster is the last cluster of a file
 * @param *vol FAT volume structure
 * @param cluster Cluster number
 * @return True if EOF, false otherwise
 */
bool FatIsClusterEof(struct FatVolume *vol, uint32_t cluster);

#define FAT_CLUSTER_VALID(vol, cluster) (((cluster) >= 2) && ((cluster) <= (vol)->clusters))
#define FAT_CLUSTER_FREE(cluster) ((cluster) == 0)

#define FAT_CLUSTER_EOF 0xFFFFFFFF


#endif