# 分支变更记录（相对于原始 master）

## 1. 目标与背景
- 新增硅液滴 θ=40° 接触角案例，强调能量/接触角诊断与可视化，支持基于时间的输出间隔，便于调参与对照实验。
- 增强润湿/毛细模型（pairwise）以更贴近目标接触角；收缩计算域减少无关体积。
- 提升运行便利性：脚本生成网格、运行、绘图；增加 OpenMP 线程可配与编译优化。

## 2. 主要变化概览
- 诊断输出：`history.csv` 增列几何/能量指标，计算接触角 `theta_deg`（2*atan(H/R)）。
- 毛细润湿：增加液-壁亲和缩放 `wetting_scale`，可通过 JSON 配置。
- 输出控制：新增 `iter_output_time`，优先按时间间隔确定输出步长，并给出提示/警告。
- 案例与脚本：`cases/si_droplet_theta40` 目录下完善生成、运行、绘图流程；`run.sh` 支持 `-omp`。
- 可视化：新增 `plot_history.py`，从 `history.csv` 出图（R/H/beta/KE/Diss/Wcap）。
- 编译：CMake 默认开启 `-O3 -march=native -ffast-math`。

## 3. 文件与目录调整
- 新增 `modify_history.md`（本文件）。
- `cases/si_droplet_theta40/`：`generate_grid.py`、`si_droplet_theta40.json`、`run.sh`、`plot_history.py`、网格/STL/Paraview 状态等。
- `output/si_droplet_theta40/`：历史输出示例 `history.csv`、VTU。
- 源码：
  - `include/MpsParticleSystem.h`
  - `src/MpsParticleSystem.cpp`
  - `src/MpsInputOutput.cpp`
  - `src/main.cpp`
  - `src/physics/PairwiseCapillary.*`
  - `src/post/Diagnostics.*`
- 构建：`CMakeLists.txt` 优化标志更新。

## 4. 关键功能改动
- Diagnostics (`src/post/Diagnostics.*`)
  - 输出列：`t,R,H,beta,theta_deg,KE,Diss,Wcap`。
  - 接触角估计：`theta_deg = 2*atan(H/R)`。
  - 累积耗散与毛细功：`Diss`、`Wcap`。
- 输出间隔 (`iter_output_time`)
  - JSON `numerical.iter_output_time`（秒）读取到 `MpsParticleSystem::iterOutputTime`。
  - 主循环优先使用基于时间的步长：`iterOutput = int(iter_output_time / time_step)`，下限 1。
  - 若非整倍，打印 Warning，告知实际间隔；整倍打印 Info。历史时间基于 `timeCurrent`，与实际间隔一致。
  - `iter_output` 仅作为回退/缺省。
- 毛细/润湿
  - 新增 `pairwiseWettingScale`（JSON `pairwise.wetting_scale`），放大液-壁吸引力。
  - `pairwiseStrengthWall = pairwiseStrength * (1 + cos(theta)) * wetting_scale`。
- 案例参数 (`cases/si_droplet_theta40/si_droplet_theta40.json`)
  - 液滴体积 3.5 pL，`particle_dist = 1e-6` m，`time_step = 5e-8` s，`final_time = 1e-4` s。
  - 域收缩：x/y ∈ [-6e-5, 6e-5] m，z ∈ [0, 8e-5] m。
  - Pairwise：`re_over_dx=2.4`，`c_sigma=0.25`，`short_clip=0.3`，`wetting_scale=2.5`。
  - 输出：`write_csv_every=5`，`write_vtk_every=10`，且 `iter_output_time=1e-6`（优先）。
- 网格生成 (`generate_grid.py`)
  - 体积 3.5 pL → 半径约 9.42 µm；粒距 1 µm；初速度 1 m/s；生成约 3k 粒子。
- 运行脚本 (`run.sh`)
  - 支持 `-omp N` 设置 `OMP_NUM_THREADS`（默认 8）。
  - 激活 `conda` 环境 `py312_mps`（若存在）。
  - 流程：生成网格 → 拷贝 JSON → 运行 `bin/main` → 调用 `plot_history.py` 生成 `history_plot.png`。
- 绘图 (`plot_history.py`)
  - 读取 `history.csv`，一列多行子图，绘制 `R/H/beta/KE/Diss/Wcap` vs 时间。

## 5. 编译与优化
- `CMakeLists.txt` 现使用 `-O3 -march=native -ffast-math`，针对本机 CPU 开启向量化与松弛浮点规则；跨机器可移植性下降，必要时移除 `-march=native`。
- 构建命令：`mkdir -p build && cd build && cmake .. && make -j`。

## 6. 使用与运行示例
- 编译：`cd build && cmake .. && make -j4`。
- 运行案例：`bash cases/si_droplet_theta40/run.sh -omp 16`。
- 产出：`output/si_droplet_theta40/history.csv`、`history_plot.png`、VTU 序列。
- 输出间隔提示会在标准输出打印 Info/Warning，确认 `iter_output_time` 已生效。

## 7.1 力与能量算法说明（Equations.md 增补）
- 增补了压力梯度、粘性、重力、碰撞/墙面排斥、pairwise 表面张力/润湿的离散公式，明确 \(A_{ll}=c_\sigma \sigma \Delta x\)、\(A_{lw}=A_{ll}(1+\cos\theta)S_w\)、裁剪距离/影响半径等。
- 说明了能量诊断：动能、粘性功率累积 `Diss`、毛细功率累积 `Wcap`，以及调试量 `maxAccCap`/`capPower` 的含义。
- 输出控制细节：`iter_output_time` 优先，若非 `time_step` 整倍则提示实际步长；历史时间使用真实 `timeCurrent`。

## 7. 已知问题/待验证
- 目标接触角 40° 的收敛仍偏高（~59°）；需要继续调节润湿参数（`wetting_scale`、`c_sigma` 等）或壁/自由面模型。
- `history.csv` 中 `Wcap` 仍可能为 0，需进一步检查毛细功累积实现。
- 更激进的编译优化可能导致不同平台数值差异，跨平台运行需评估。

## 8. 输入案例整理与运行计划
- 现有 `input/` 示例分组：  
  - Dam break (Lobovský)：`MpsInputExample.json`、`InputDamINC.json`、`InputDamWC.json`、`dam1610_h300_lo0p0050_INC/WC.json`  
  - 其他 Dam：`InputDamWall_INC.json`、`dam1610_BC.json`  
  - Tailings (Brumadinho)：`BRUMADINHO_space10_lo10p00.json`  
  - 非牛顿 Fraccarollo：`damErosion3D_WC.json`、`2D_dam_fraccarollo_lo02p50e-03_INC/WC.json`  
  - 非牛顿 Nodoushan：`S1_2D_lo08p00e-04_INC.json`、`subaerial_2D_lo08p00e-04_INC.json`、`subaquatic_2D_lo08p00e-04_INC/WC.json`  
  - 其他：`sloshing_3D_bulian_lo0p00930.json`、`si_droplet_theta40.json`（新增液滴案例）
- 整理方案（待实施）：  
  1) 建立 `tests/cases.yaml`（或 `input/cases.yaml`），为每个案例定义别名、JSON 路径、维度、INC/WC、是否需 grid 解压、建议输出目录。  
  2) 添加 `scripts/polymps-test.sh`（或二进制包装 `polymps test <name>`），支持 `-omp N`、`--output`、`--binary`、`--list`，自动检查 `input/grid` 是否解压并运行对应 JSON。  
  3) 在 README/本文件补充使用示例：`./scripts/polymps-test.sh dam_fraccarollo2d_wc -omp 8` 等。
