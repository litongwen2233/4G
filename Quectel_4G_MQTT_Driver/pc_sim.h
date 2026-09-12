#ifndef PC_SIM_H
#define PC_SIM_H

/* PC 端演示用：向“虚拟模组”注入一条服务器下行的 URC（如 +QMTRECV）。
 * 仅用于在没有真实 4G 模组时跑通流程；接真机时连同 pc_sim.c 一起删除。 */
void pc_sim_inject_recv(const char *urc);

#endif /* PC_SIM_H */
