#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
BIN="${BUILD_DIR}/bin/main"
CASE_NAME="si_droplet_theta40"
OUTPUT_DIR="${ROOT_DIR}/output/${CASE_NAME}"
OMP_THREADS_DEFAULT=8
USER_OMP=""

# Parse simple CLI args: -omp N
while [[ $# -gt 0 ]]; do
	case "$1" in
		-omp)
			USER_OMP="$2"
			shift 2
			;;
		*)
			echo "Unknown option: $1"
			exit 1
			;;
	esac
done

if [[ -n "${USER_OMP}" ]]; then
	OMP_NUM_THREADS=${USER_OMP}
else
	OMP_NUM_THREADS=${OMP_NUM_THREADS:-$OMP_THREADS_DEFAULT}
fi

# Activate conda environment if available (best-effort, no failure on errors)
if command -v conda >/dev/null 2>&1; then
	set +e
	CONDA_NO_PLUGINS=true BASE_CONDA_PATH=$(CONDA_NO_PLUGINS=true conda info --base 2>/dev/null)
	if [ -n "${BASE_CONDA_PATH:-}" ] && [ -f "${BASE_CONDA_PATH}/etc/profile.d/conda.sh" ]; then
		# shellcheck disable=SC1090
		source "${BASE_CONDA_PATH}/etc/profile.d/conda.sh"
		conda activate py312_mps >/dev/null 2>&1 || true
	elif [ -f "${HOME}/opt/miniconda3/etc/profile.d/conda.sh" ]; then
		# shellcheck disable=SC1090
		source "${HOME}/opt/miniconda3/etc/profile.d/conda.sh"
		conda activate py312_mps >/dev/null 2>&1 || true
	fi
	set -e
fi

python "${SCRIPT_DIR}/generate_grid.py"

# Copy JSON into the input folder expected by PolyMPS
cp "${SCRIPT_DIR}/${CASE_NAME}.json" "${ROOT_DIR}/input/${CASE_NAME}.json"

if [ ! -x "${BIN}" ]; then
	echo "Binary ${BIN} not found. Please build with: mkdir -p ${BUILD_DIR} && cd ${BUILD_DIR} && cmake .. && make -j"
	exit 1
fi

cd "${ROOT_DIR}"
echo "${CASE_NAME}" | OMP_NUM_THREADS=${OMP_NUM_THREADS} "${BIN}"

# Plot R/H/beta/KE/Diss/Wcap from history.csv if available
python "${SCRIPT_DIR}/plot_history.py"
