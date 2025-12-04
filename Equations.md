# 模型与方程说明（PolyMPS 摘要）

本文档概述当前分支使用的主要物理/数值假设、势函数构成、控制方程、离散化步骤与初始条件设置，便于回溯实现逻辑。

## 1. 基本假设与粒子设定
- 连续介质假设：流体视为可压/弱可压或近似不可压的连续介质，属性在粒子上存储并随流动拉格朗日移动。
- 粒子表示：每个粒子代表体积 \( \Delta V \approx \text{partDist}^\text{dim} \)（3D 则为立方体等效），携带质量 \( m_i = \rho_i \Delta V \)、速度、位置等。
- 近邻作用：以影响半径 \( r_e \)（小/大核）和核函数 \( w(r) \) 估算梯度、拉普拉斯、PND 等。
- 边界：可用粒子壁（wall/dummy）或多边形壁（STL），支持周期/固定等边界条件。

## 2. 势函数与表面张力（Pairwise Capillary）
- 液-液作用强度 \( A_{ll} = C_\sigma \, \sigma \, \Delta x \)，液-壁强度 \( A_{lw} = A_{ll} (1+\cos\theta) S_w \)，其中 \( \sigma \) 为表面张力，\( \theta \) 为目标接触角，\( S_w \) 为润湿缩放（`wetting_scale`）。
- 作用范围 \( r < r_e = (\text{re_over_dx}) \Delta x \)，短程裁剪系数 `short_clip` 用于削弱接触附近的力尖峰。
- 势能形式采用对称势，力为势梯度；累积功用于 `Wcap` 诊断。

## 3. 控制方程（连续形式）
- 质量守恒（弱可压或显式密度传播）：\( \frac{D\rho}{Dt} = -\rho \nabla\cdot \mathbf{u} \) 或通过 EOS 近似压缩性。
- 动量方程：\( \frac{D\mathbf{u}}{Dt} = -\frac{1}{\rho}\nabla p + \nu \nabla^2 \mathbf{u} + \mathbf{g} + \mathbf{f}_\text{cap} + \mathbf{f}_\text{coll} + \mathbf{f}_\text{wall} \)
  - \( \mathbf{f}_\text{cap} \)：由 pairwise 势导出的表面张力/润湿力。
  - \( \mathbf{f}_\text{coll} \)：颗粒碰撞/排斥（可选）。
  - \( \mathbf{f}_\text{wall} \)：墙面排斥、镜像压力、边界条件修正。

## 4. 离散与数值步骤（主循环）
1. 显示/输出控制：依据 `iter_output_time` 或 `iter_output` 写粒子/网格/历史文件。
2. 更新桶和邻域：构建空间散列表，统计邻居。
3. 体积分数/粘度（非牛顿可选）：计算混合物体积分数与等效粘度。
4. 粘性项：拉普拉斯近似计算 \( \nu \nabla^2 \mathbf{u} \)。
5. 重力：加到加速度。
6. 压力预测梯度：若半隐式，先用松弛预测压力梯度；含墙面梯度修正。
7. 预测步：更新预测速度/位置；重置部分中间量。
8. 出界检查：周期或裁剪处理。
9. 碰撞处理：PC 或动态碰撞模型。
10. 墙面贡献：镜像/PND 修正/壁压力等。
11. 自由面检测：基于 PND/邻居/角度等判据。
12. PND 计算与平滑（用于压力泊松）。
13. 压力求解：显式/弱可压或半隐式泊松（迭代求解器）。
14. 速度-位置校正：用压力梯度、粘度、体积力、表面张力等更新速度、位置。
15. 粒子位移调整（shifting，可选）：减缓数值聚团。
16. 诊断采样：几何尺度 \( R,H,\beta \)、接触角 \( \theta=2\arctan(H/R) \)、动能、累积耗散 `Diss`、毛细功 `Wcap`。

## 5. 力与能量的离散定义（诊断口径）
- 粒子质量 \( m_i = \rho_i \Delta V \)，其中 \( \Delta V = \text{partDist}^{\text{dim}} \)。
- 动能：\( KE = \sum_i \tfrac12 m_i |\mathbf{u}_i|^2 \)。
- 粘性功率：\( P_\nu = \sum_i m_i (\mathbf{a}_{\nu,i}\cdot \mathbf{u}_i) \)，累积耗散 `Diss = \int -P_\nu \, dt`（若为负则钳至 0）。
- 毛细功率：\( P_\text{cap} = \sum_i m_i (\mathbf{a}_{\text{cap},i}\cdot \mathbf{u}_i) \)，累积 `Wcap = \int P_\text{cap} \, dt`。
- 液滴几何：质心投影下的最大半径 \( R = \max\sqrt{(x-c_x)^2+(y-c_y)^2} \)，高度 \( H = z_\text{max} - z_\text{substrate} \)，铺展系数 \( \beta = R/R_0 \)（初始 \( R_0 \) 由首帧估计）。
- 接触角估计：\( \theta = 2\arctan(H/R) \)（诊断用）。

### 力项的具体算法
- 压力梯度：采用 MPS 梯度核 \( \nabla p \approx \frac{d}{n_0} \sum_j \frac{p_j - p_i}{|r_{ij}|^2} w(|r_{ij}|) r_{ij} \)，并含壁面镜像/多边形校正。
- 粘性：拉普拉斯近似 \( \nu \nabla^2 \mathbf{u} \approx \frac{2d\nu}{\lambda n_0} \sum_j (\mathbf{u}_j - \mathbf{u}_i) w(|r_{ij}|) \)，非牛顿时先估等效黏度。
- 重力：\( \mathbf{g} \) 直接加到加速度。
- 碰撞/排斥：基于最小间距阈值的恢复系数模型（PC 或动态），墙面排斥力按选定模型（Mitsume/LJ/Monaghan-Kajtar）计算，作用距离与 `wall_repulsive_force.re`/`maxVel` 等有关。
- 表面张力/润湿（pairwise）：
  - 强度：\( A_{ll} = c_\sigma \, \sigma \, \Delta x \)，\( A_{lw} = A_{ll}(1+\cos\theta) S_w \)。
  - 影响半径：\( r_e = (\text{re_over_dx}) \Delta x \)，裁剪距离 \( r_\text{clip} = \text{short_clip} \cdot \Delta x \)。
  - 权重：\( w(r) = r_e/r - 1 \)（当 \( r < r_e \)），力 \( \mathbf{f}_{ij} = \pm A\, w(r)/\max(r,r_\text{clip}) \, \hat{r}_{ij} \)，粒子加速度为 \(\mathbf{a}_i = \mathbf{f}_{ij}/m_i\)。
  - 液-液作用遍历邻域；液-壁作用使用最近多边形点（若 nearWall=true）。

### 能量与输出
- `maxAccCap`（调试）记录一步内 capillary 加速度模最大值；`capPower` 为上述功率总和。
- 输出时间步由 `iter_output_time` 决定；若非 `time_step` 整倍，则按取整步数并打印提示。

## 6. 时间推进
- 时间步长 `time_step = dt`，满足 CFL、黏性扩散与表面张力等稳定性要求。
- 弱可压情况：EOS \( p = c_s^2 (\rho-\rho_0) \)（`speed_sound`, `gamma`）或半隐式泊松。
- 输出控制：若 `iter_output_time>0`，输出步长 \( n = \max(1, \lfloor \text{iter_output_time}/dt \rfloor) \)；若非整倍打印警告；历史时间使用真实仿真时间 `timeCurrent`。

## 7. 初始条件
- 粒子生成：规则网格或装箱体积，粒距 `partDist`，赋予初始速度（如液滴 `U0`）。
- 密度/压力：初值 \( \rho_0 \)，压强可设为 0 或静水分布。
- 边界：墙体（粒子或多边形）预先放置，必要时定义周期长度。
- 接触角/润湿：在 JSON 中设 `substrate.contact_angle_deg` 与 `pairwise.wetting_scale`，表面张力 `surface_tension`。
- 域与重力：`domain_min/max` 确定计算箱，`gravity` 向量设定体力。

## 8. 关键参数对照（与实现相关）
- `partDist`：粒距；`re_over_dx`：pairwise 影响半径系数；`c_sigma`：pairwise 力强度系数。
- `short_clip`：短程裁剪；`wetting_scale`：液-壁亲和放大；`contact_angle_deg`：目标接触角。
- `time_step`、`final_time`、`iter_output_time`、`iter_output`、`write_csv_every`：时间积分与输出控制。
- `CFL_number`、`relax_fact`、`gradient.type/correction`：数值稳定性与压力求解设置。
- `wall_repulsive_force`、`particle_collision`：碰撞/排斥模型选择与强度。

## 9. 适用范围与限制
- 适合中等粒数的 CPU/OpenMP 场景；GPU/NPU 未实现。
- `-ffast-math` + `-march=native` 可能带来平台相关的数值差异；跨平台需重新编译或移除这些选项。
- 诊断接触角采用几何尺度估计，非严格拟合界面，近似用于收敛趋势判断。
