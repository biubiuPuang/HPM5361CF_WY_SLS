#ifndef IDS830ABS_REGISTERS_H
#define IDS830ABS_REGISTERS_H

/*
 * IDS830ABS RS485/Modbus RTU 寄存器常量。
 * 坐标方向约定：负 cnt = 前进/伸出/顶踏板；正 cnt = 后退/缩回/离开踏板。
 */

#define IDS_REG_ENABLE             0x00u
#define IDS_REG_MODE_SELECT        0x02u
#define IDS_REG_TORQUE_TARGET      0x08u
#define IDS_REG_POS_ACCEL          0x09u
#define IDS_REG_VEL_ACCEL          0x0Au
#define IDS_REG_VEL_TARGET         0x10u
#define IDS_REG_COMM_TIMEOUT       0x1Cu
#define IDS_REG_MAX_VEL_LIMIT      0x1Du
#define IDS_REG_CTRL_STATE         0x36u
#define IDS_REG_CLEAR_FAULT        0x4Au
#define IDS_REG_MULTI_TURN_CLEAR   0x4Cu
#define IDS_REG_EMG_STOP           0x4Du
#define IDS_REG_BUF_STOP           0x4Fu
#define IDS_REG_POS_HIGH           0x50u
#define IDS_REG_ABS_REL            0x51u

#define IDS_M_R_FAST_MONITOR       0xE3u
#define IDS_M_R_FEEDBACK_POS       0xE8u

#define IDS_MODE_TORQUE_PC         0x00C1u
#define IDS_MODE_VEL_PC            0x00C4u
#define IDS_MODE_POS_PC            0x00D0u

#define IDS_CTRL_STATE_PC          0x0002u
#define IDS_COMM_TIMEOUT_ENABLE    0x0007u
#define IDS_COMM_TIMEOUT_DISABLE   0x0000u

#define IDS_STATUS_RUNNING_BIT     0u
#define IDS_STATUS_OVER_CURRENT    1u
#define IDS_STATUS_OVER_VOLTAGE    2u
#define IDS_STATUS_ENCODER_ERROR   3u
#define IDS_STATUS_POSITION_ERROR  4u
#define IDS_STATUS_UNDER_VOLTAGE   5u
#define IDS_STATUS_OVERLOAD        6u
#define IDS_STATUS_EXTERNAL_CTRL   7u

#endif /* IDS830ABS_REGISTERS_H */
