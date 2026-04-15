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

<img src="images/1.png" width="1000"/>


*multiple containers running (cpu_hog, memory_hog, io_pulse).*

---

### 3.2 **Metadata Tracking**

**Description:** Listing running containers and their PIDs.

<img src="images/2.png" width="1000"/>


*`./engine list` or `ps -ef | grep hog`.*

---

### 3.3 **Logging System**

**Description:** Container lifecycle events recorded in log file.

<img src="images/3.png" width="1000"/>


*`cat runtime.log` output.*

---

### 3.4 **CLI and IPC**

**Description:** CLI command triggers supervisor, kernel module receives PID via ioctl.

<img src="images/4.png" width="1000"/>

<img src="images/5.png" width="1000"/>


*CLI command is issued from user-space, and the kernel module receives the process information via ioctl, demonstrating interaction between user-space and kernel-space.*

---

### 3.5 **Soft-limit Warning**

**Description:** Warning generated when memory usage crosses a defined threshold.

<img src="images/6.png" width="1000"/>

*Soft limit warning shown in runtime logs.*

---

### 3.6 **Hard-limit Enforcement**

**Description:** Container termination when usage exceeds maximum threshold.

<img src="images/7.png" width="1000"/>


*Container kill message shown in logs.*

---

### 3.7 **Scheduling Experiment**

**Description:** CPU, memory, and I/O workloads comparison.

<img src="images/8.png" width="1000"/>

<img src="images/9.png" width="1000"/>

*`top`, `free -h`, `ps` outputs.*

---

### 3.8 **Clean Teardown**

**Description:** Containers stopped with no zombie processes.

<img src="images/10.png" width="1000"/>

*After stopping containers, `ps aux | grep engine` shows no running processes.*

---

## 4. 🧠 Engineering Analysis

* Linux namespaces (`CLONE_NEWPID`, `CLONE_NEWUTS`, `CLONE_NEWNS`) provide process isolation.
* `chroot()` enables filesystem isolation inside containers.
* Kernel modules operate in privileged mode for system monitoring.
* `ioctl` is used for communication between user-space and kernel-space.
* Linux scheduler allocates CPU based on workload behavior.
* The project demonstrates how lightweight containerization can be built using core Linux system calls without full container engines like Docker.

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

* Built a lightweight container runtime using C and Linux system calls  
* Achieved process and filesystem isolation using namespaces and chroot  
* Integrated kernel-space monitoring using a loadable kernel module  
* Demonstrated scheduling behavior using CPU, memory, and I/O workloads

---
