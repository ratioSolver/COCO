# CoCo: Combined Deduction and Abduction Reasoner

![Build Status](https://github.com/ratioSolver/COCO/actions/workflows/cmake.yml/badge.svg)
[![codecov](https://codecov.io/gh/ratioSolver/COCO/branch/master/graph/badge.svg)](https://codecov.io/gh/ratioSolver/COCO)

**CoCo** (Combined deduCtiOn and abduCtiOn) is a dual-process inspired cognitive architecture built in Rust. It integrates a rule-based expert system and a timeline-based planner to invoke deductive and abductive reasoning in dynamic environments.

The system leverages [CLIPS](https://www.clipsrules.net) for its robust pattern matching and rule engine capabilities, handling dynamic changes effectively.

## Features
- **Hybrid Reasoning**: Unites deductive logic with abductive inference.
- **Rust Core**: Designed for performance, memory safety, and concurrency.
- **CLIPS Integration**: Seamless binding with the C-based CLIPS expert system.
- **Web Interface**: Includes a web server (Axum) and visualization tools.

## Usage

To run the CoCo server:

```bash
cargo run --bin server
```

The server listens on `http://0.0.0.0:3000` and serves the web interface. 


## Installation

### Prerequisites
CoCo relies on **CLIPS v6.4.2** C libraries.

### Installing CLIPS
Follow these steps to install the required CLIPS library and headers on your local machine:

1. **Download**:
   Download [CLIPS v6.4.2](https://sourceforge.net/projects/clipsrules/files/CLIPS/6.4.2/clips_core_source_642.zip/download) and unzip the archive.
   ```bash
   wget -O clips_core_source_642.zip https://sourceforge.net/projects/clipsrules/files/CLIPS/6.4.2/clips_core_source_642.zip/download
   unzip clips_core_source_642.zip
   ```

2. **Compile**:
   Navigate to the core source directory and compile:
   ```bash
   cd clips_core_source_642/core
   make release
   ```

3. **Install Library**:
   Set the `CLIPS_SOURCE_DIR` environment variable to the path of the CLIPS source directory:
   ```bash
   export CLIPS_SOURCE_DIR=$(pwd)
   ```
   Alternatively, for a global installation:
   ```bash
   sudo cp libclips.a /usr/local/lib/
   sudo ldconfig
   ```