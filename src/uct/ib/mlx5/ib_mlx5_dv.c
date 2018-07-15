/**
* Copyright (C) Mellanox Technologies Ltd. 2001-2018.  ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#include "ib_mlx5.h"
#include "ib_mlx5_log.h"

#if HAVE_DECL_MLX5DV_INIT_OBJ
ucs_status_t uct_ib_mlx5dv_init_obj(uct_ib_mlx5dv_t *obj, uint64_t type)
{
    int ret;

    ret = mlx5dv_init_obj(&obj->dv, type);
    if (ret != 0) {
        ucs_error("DV failed to get mlx5 information. Type %lx.", type);
        return UCS_ERR_NO_DEVICE;
    }

    return UCS_OK;
}
#endif

ucs_status_t uct_ib_mlx5dv_query(void *ctx)
{
#if HAVE_DECL_MLX5DV_QUERY_DEVICE
    uct_ib_device_t *dev = (uct_ib_device_t *)ctx;
    const uct_ib_device_spec_t *spec;
    int ret;

    spec = uct_ib_device_spec(dev);
    if (!(spec->flags & UCT_IB_DEVICE_FLAG_MLX5_PRM)) {
        return UCS_OK;
    }

    ret = mlx5dv_query_device(dev->ibv_context, &dev->dv_attr.mlx5);
    if (ret != 0) {
        ucs_error("mlx5dv_query_device() returned %d: %m", ret);
        return UCS_ERR_IO_ERROR;
    }
    dev->dv_attr.provider = UCT_IB_PROVIDER_MLX5;
#endif

    return UCS_OK;
}

enum {
    UCT_IB_MLX5_CQ_SET_CI  = 0,
    UCT_IB_MLX5_CQ_ARM_DB  = 1,
};

int uct_ib_mlx5dv_arm_cq(uct_ib_mlx5_cq_t *cq, int solicited)
{
    uint64_t doorbell;
    uint32_t sn;
    uint32_t ci;
    uint32_t cmd;

    sn  = cq->cq_sn & 3;
    ci  = cq->cq_ci & 0xffffff;
    cmd = solicited ? MLX5_CQ_DB_REQ_NOT_SOL : MLX5_CQ_DB_REQ_NOT;

    cq->dbrec[UCT_IB_MLX5_CQ_ARM_DB] = htobe32(sn << 28 | cmd | ci);

    ucs_memory_cpu_fence();

    doorbell = sn << 28 | cmd | ci;
    doorbell <<= 32;
    doorbell |= cq->cq_num;

    *(uint64_t *)((uint8_t *)cq->uar + MLX5_CQ_DOORBELL) = htobe64(doorbell);

    ucs_memory_bus_store_fence();

    return 0;
}

#if !HAVE_INFINIBAND_MLX5_HW_H
void uct_ib_mlx5_update_cq_ci(struct ibv_cq *cq, unsigned cq_ci) { }

unsigned uct_ib_mlx5_get_cq_ci(struct ibv_cq *cq) { return 0; }

#endif

#if HAVE_DECL_MLX5DV_GET_AV
void uct_ib_mlx5_get_av(struct ibv_ah *ah, struct mlx5_wqe_av *av)
{
    mlx5dv_get_av(ah, av);
}
#endif

