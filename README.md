# 🧠 Multi-Container Runtime with Kernel Monitoring

---

## 1. 👥 Team Information

### Member 1
* **Name:** KOTRA SAI SOUMYA SRI
* **SRN:**  PES1UG24CS911


### Member 2
* **Name:** TANMAYI NAGABHAIRAVA
* **SRN:**  PES1UG24CS493

---

## 2. ⚙️ Build, Load, and Run Instructions

### 🔹 Environment Setup

* Ubuntu 22.04 / 24.04 VM
* Secure Boot OFF
* Install dependencies:

```bash
sudo apt update
sudo apt install -y build-essential linux-headers-$(uname -r)
```

---

### 🔹 Build Project

```bash
cd boilerplate
make clean
make
```

---

### 🔹 Load Kernel Module

```bash
sudo insmod monitor.ko
lsmod | grep monitor
```

---

### 🔹 Run Containers

```bash
sudo ./engine run alpha ./cpu_hog
sudo ./engine run beta ./cpu_hog
```

---

### 🔹 List Containers

```bash
./engine list
```

---

### 🔹 Run Workloads

```bash
./cpu_hog
./memory_hog
./io_pulse
```

---

### 🔹 Stop Containers

```bash
./engine stop alpha
./engine stop beta
```

---

### 🔹 View Logs

```bash
cat runtime.log
dmesg | tail
```

---

### 🔹 Cleanup

```bash
sudo rmmod monitor
```

---

## 3. 📸 Demo with Screenshots

---

### 3.1 **Multi-container Supervision**

**Description:** Two or more containers running simultaneously.

<img width="851" height="506" alt="n5" src="https://github.com/user-attachments/assets/e3b97721-eab2-4718-a4ed-2ae7a7fa22eb" />


*multiple containers running (cpu_hog, memory_hog, io_pulse).*

---

### 3.2 **Metadata Tracking**

**Description:** Listing running containers and their PIDs.

<img width="788" height="135" alt="n4" src="https://github.com/user-attachments/assets/b19288c9-516d-401b-8684-6616ac6b22ff" />


*`./engine list` or `ps -ef | grep hog`.*

---

### 3.3 **Logging System**

**Description:** Container lifecycle events recorded in log file.

<img width="822" height="168" alt="3 1" src="https://github.com/user-attachments/assets/00768d55-3a6a-4811-ae12-f8bc0cf9069b" />


*`cat runtime.log` output.*

---

### 3.4 **CLI and IPC**

**Description:** CLI command triggers supervisor, kernel module receives PID via ioctl.

<img width="1060" height="328" alt="n10" src="https://github.com/user-attachments/assets/e97fc44d-eff4-4abf-bacd-02de23e2cc77" />

<img width="811" height="97" alt="n3" src="https://github.com/user-attachments/assets/49c64f9d-8e90-4174-a35b-1a115f8d6ab2" />



*`engine run` + `dmesg | tail`.*

---

### 3.5 **Soft-limit Warning**

<img width="767" height="91" alt="image" src="https://github.com/user-attachments/assets/90847ee1-f105-42de-bf7b-da661f193596" />


---

### 3.6 **Hard-limit Enforcement**

<img width="767" height="75" alt="image" src="https://github.com/user-attachments/assets/52fdf964-c999-4ded-be3c-2582095d899e" />


---

### 3.7 **Scheduling Experiment**

**Description:** CPU, memory, and I/O workloads comparison.

<img width="1049" height="651" alt="n6" src="https://github.com/user-attachments/assets/574977c8-4c37-493b-bf16-400a3dd31aa6" />


<img width="808" height="133" alt="n9" src="https://github.com/user-attachments/assets/686e336d-501d-4199-a2b9-61e91030aae3" />


*`top`, `free -h`, `ps` outputs.*

---

### 3.8 **Clean Teardown**

**Description:** Containers stopped with no zombie processes.

<img width="918" height="161" alt="2" src="https://github.com/user-attachments/assets/3f07b0da-859f-4c10-a558-3e7c49028cb4" />


* after this, `ps aux | grep engine` shows no running processes.

---

## 4. 🧠 Engineering Analysis

* Linux namespaces (`CLONE_NEWPID`, `CLONE_NEWUTS`, `CLONE_NEWNS`) provide process isolation.
* `chroot()` enables filesystem isolation inside containers.
* Kernel modules operate in privileged mode for system monitoring.
* `ioctl` is used for communication between user-space and kernel-space.
* Linux scheduler allocates CPU based on workload behavior.

---

## 5. ⚖️ Design Decisions and Tradeoffs

### 🔹 Namespace Isolation

* **Choice:** Used `clone()` with namespaces
* **Tradeoff:** Limited compared to full container runtimes
* **Reason:** Simpler implementation

---

### 🔹 Supervisor Architecture

* **Choice:** CLI-based supervisor
* **Tradeoff:** No persistent state
* **Reason:** Reduced complexity

---

### 🔹 IPC and Logging

* **Choice:** `ioctl` + file logging
* **Tradeoff:** Minimal communication features
* **Reason:** Lightweight

---

### 🔹 Kernel Monitor

* **Choice:** Basic PID tracking
* **Tradeoff:** No memory enforcement
* **Reason:** Focus on integration

---

### 🔹 Scheduling Experiments

* **Choice:** CPU, memory, I/O workloads
* **Tradeoff:** No detailed metrics
* **Reason:** Clear observable behavior

---

## 6. 📊 Scheduler Experiment Results

### 🔹 Observations

| Workload   | Behavior              |
| ---------- | --------------------- |
| CPU Hog    | High CPU usage        |
| Memory Hog | High memory usage     |
| IO Pulse   | Intermittent activity |

---

### 🔹 Analysis

* CPU-bound processes dominate CPU time
* Memory-heavy processes increase RAM usage
* I/O workloads show burst patterns
* Scheduler balances processes dynamically

---

## 🎯 Conclusion

* Built a basic container runtime using C
* Achieved process isolation using Linux primitives
* Integrated kernel module with user-space runtime
* Demonstrated scheduling behavior using workloads

---
