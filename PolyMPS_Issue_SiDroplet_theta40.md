# [Feature] Si-substrate microdroplet (θ=40°) on PolyMPS — Potential surface tension + wetting, β/R/H & energy outputs, ParaView one 

> 分支：`feature/droplet_on_Si_theta40`  
> 基座：PolyMPS（C++/OpenMP, MIT）

---

## 🎯 目标（Goal）
基于 **PolyMPS**，新增并集成：
1) **粒间势表面张力（Kondo‑style inter‑particle potential）**；  
2) **润湿/接触角设定**（液‑固 vs 液‑液势强度比映射 Young 关系；目标 θ=40°，Si 平面基板）；  
3) 观测/导出 **β(t)=R/R₀、R(t)、H(t)**；  
4) **能量通道分解**（动能 KE、黏性耗散功 Diss、势力做功 Wcap 作为界面能代理）；  
5) 提供 **ParaView 一键可视化**（`.pvsm`），加载即出几何视图 + β/H/能量曲线。

初始算例：**3.5 pL 水滴、U₀=2.5 m/s、θ=40°、Si 平面，轴向重力**。

---

## 📚 背景/依据（必须遵循）
- **PolyMPS**：MPS 求解器，支持**多边形（三角面片）墙**与**不可压/弱可压**两路，适合自由表面与复杂几何。  
  - 仓库与编译说明（GitHub）。  
  - 软件论文（Software Impacts, 2022），明确“Polygon wall in MPS”。  
- **Kondo–Koshizuka（2007）势模型**：用**粒间势**实现表面张力，并给出与宏观 **σ/θ** 的对应关系；是“Potential model + 接触角设定”的理论基础。  
- **Particleworks（工业 MPS 对标）**：官方资料与宣传册明确 **Potential model 可直接设定接触角**，并使用**Polygon wall/权函数**等术语，可作为功能对齐参照。  
- **ParaView 状态文件**：`.pvsm` 为 XML‑based 的**最稳健**状态保存方式（Save/Load State），可共享复现管线。

> 以上出处见文末“参考链接”。

---

## 🧭 范围与技术要求（Scope / Requirements）
- **Pairwise 势核（液‑液）**  
  - 影响半径 `re ≈ (2.0–2.5) Δx`；权函数 `w = re/r − 1`（0<r<re）；**短程钳制** `r_eff ≥ 0.3 Δx`。  
  - 势强度：`A_ll = c_sigma · σ · Δx`（起步 `c_sigma ≈ 0.25`）；力项 `F_ij = (A · w) (d_ij / r_eff)`。
- **润湿/接触角（液‑固）**  
  - 在多边形墙邻域施加同形势：`A_wl = A_ll (1 + cos θ)`，以 **Young** 关系映射目标接触角（本案 θ=40°）。  
- **自由面条件**  
  - 维持 PolyMPS 既有数密度阈值 `n < α n0` 的处理，自由面施加 `p=0`（Dirichlet）。
- **观测/导出**  
  - 每 `N_csv` 步写 `history.csv`（列：`t, R, H, β, KE, Diss, Wcap`）；  
  - 每 `N_vtk` 步输出 VTK（点坐标/速度/自由面标记/相位），便于 ParaView。
- **ParaView**  
  - 提交一个 `.pvsm`，加载 `history.csv` + `fields_*.vtu`，显示**侧视/俯视**几何 + **β/H/能量**曲线（Plot Over Time）。

---

## 🧩 代码基与分支（Repo & Branch）
- 基仓：`https://github.com/rubensamarojr/polymps`（MIT）。  
- 派生并创建：`feature/droplet_on_Si_theta40`。  
- 不得重写底层时间推进/求解框架；以**增量模块**形式添加。

---

## 🛠️ 实施清单（Task List）
- [ ] 新增 `src/physics/PairwiseCapillary.{h,cpp}`：粒间势核与液‑液/液‑固力计算  
- [ ] 在受力汇总中挂接 **Fi_cap**（液‑液/液‑固两路），并保留 **Fi_visc**、体力等原有通道  
- [ ] 新增 `src/post/Diagnostics.{h,cpp}`：β/R/H 与能量分解（KE、Diss、Wcap）  
- [ ] 新增/修改 CMake：加入上述新源文件  
- [ ] 新增案例目录 `cases/si_droplet_theta40/`：`input.yaml`、`si_flat.stl`、`run.sh`、`paraview/si_drop_theta40.pvsm`  
- [ ] 输出产物：`output/history.csv` 与 `output/fields_*.vtu`  
- [ ] README 追加：如何编译/运行此案例，ParaView 加载 `.pvsm` 步骤图

---

## ✅ 验收标准（Acceptance Criteria）
**功能**  
A. β(t)、R(t)、H(t) 正常输出，无 NaN；  
B. θ=40° 静态铺展的等效接触角（图像测量）与设定角一致 ±3°；  
C. 3.5 pL、U₀=2.5 m/s 案例稳定跑至 *t* = 8×10⁻⁵ s；  
D. ParaView `.pvsm` 加载即出图（两视窗 + 曲线）。

**对标（可选）**  
E. Rayleigh 振荡频率误差 ≤10%；  
F. 最大铺展因子 β_max 与实验/参考值偏差 ≤10%。

**工程**  
G. 通过 Release 构建；注释与风格满足仓库规范。

---

## 🗺️ 里程碑（Milestones）
1. D0：拉起分支与编译通过  
2. D1：完成 Pairwise 核 & 液‑液力接入，跑 Rayleigh 验证  
3. D2：接入液‑固润湿，静态接触角回标 θ=40°  
4. D3：完成 β/R/H/能量导出 + ParaView `.pvsm`  
5. D4：3.5 pL 实算 + 报告（曲线与截屏）

---

## ⚠️ 约束与风险（Constraints/Risks）
- **稳定性**：小 Δx / 大 U₀ 时毛细‑惯性耦合刚性增强，必要时减小 Δt 或提高 PPE 预条件；  
- **几何/墙**：STL 法向指向流体侧、单位统一（m）；Polygon wall 的距离/权函数遵循 PolyMPS 实现；  
- **能量代理**：Wcap 为“势做功”代理；若需真实界面能，后续用等值面/alpha‑shape 估面积 × σ。

---

## 🧪 输入参数样例（`cases/si_droplet_theta40/input.yaml`）
```yaml
fluid: {rho: 997.0, mu: 8.9e-4, sigma: 0.072, volume_pl: 3.5, impact_speed: 2.5}
substrate: {stl: "si_flat.stl", contact_angle_deg: 40.0}
pairwise: {re_over_dx: 2.2, c_sigma: 0.25, short_clip: 0.3}
free_surface: {alpha_n0: 0.90}
time: {dt: auto, t_end: 8.0e-5}
output: {write_csv_every: 5, write_vtk_every: 10}
```


## 🚀 运行与可视化（Runbook）
```bash
mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j
cd ../cases/si_droplet_theta40 && bash run.sh
# ParaView: File -> Load State -> paraview/si_drop_theta40.pvsm
```


## 🔗 参考链接（必须阅读）
- PolyMPS GitHub（源码/编译）：https://github.com/rubensamarojr/polymps  
- PolyMPS 软件论文（Software Impacts, 2022）：https://www.sciencedirect.com/science/article/pii/S266596382200080X  
- Kondo, Koshizuka, Takimoto (2007)：  
  - ASME/FEDSM（会议页）：https://asmedigitalcollection.asme.org/FEDSM/proceedings/FEDSM2007/42886/93/327620  
  - ASME PDF（会议文集）：https://asmedigitalcollection.asme.org/FEDSM/proceedings-pdf/FEDSM2007/42886/93/2669217/93_1.pdf  
  - CRID（JSCES 条目）：https://cir.nii.ac.jp/crid/1362544420783591936  
- Particleworks（Potential 可设接触角；Polygon wall/术语）：  
  - 产品手册（PDF）：https://motionport.com/wp-content/uploads/Particleworks_E.pdf  
  - 宣传册（PDF）：https://powersys-solutions.com/Ressources/pdf/Particleworks-CFD-Brochure.pdf  
- ParaView 状态（.pvsm）：  
  - 官方手册（5.9）：https://docs.paraview.org/en/v5.9.1/UsersGuide/savingResults.html
"
