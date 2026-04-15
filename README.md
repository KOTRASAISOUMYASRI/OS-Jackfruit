# 🧠 Multi-Container Runtime with Kernel Monitoring

---

## 1. 👥 Team Information

###Member 1
* **Name:** KOTRA SAI SOUMYA SRI
* **SRN:**  PES1UG24CS911
###Member 2
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

📸 **[Insert Screenshot Here]**
<img width="851" height="506" alt="n5" src="https://github.com/user-attachments/assets/e3b97721-eab2-4718-a4ed-2ae7a7fa22eb" />

*Show multiple containers running (cpu_hog, memory_hog, io_pulse).*

---

### 3.2 **Metadata Tracking**

**Description:** Listing running containers and their PIDs.

📸 **[Insert Screenshot Here]**
*Show `./engine list` or `ps -ef | grep hog`.*

---

### 3.3 **Logging System**

**Description:** Container lifecycle events recorded in log file.

📸 **[Insert Screenshot Here]**
*Show `cat runtime.log` output.*

---

### 3.4 **CLI and IPC**

**Description:** CLI command triggers supervisor, kernel module receives PID via ioctl.

📸 **[Insert Screenshot Here]**
*Show `engine run` + `dmesg | tail`.*

---

### 3.5 **Soft-limit Warning**

**Description:** Not fully implemented.

📸 **[Insert Screenshot Here – Optional]**

---

### 3.6 **Hard-limit Enforcement**

**Description:** Not implemented.

📸 **[Insert Screenshot Here – Optional]**

---

### 3.7 **Scheduling Experiment**

**Description:** CPU, memory, and I/O workloads comparison.

📸 **[Insert Screenshot Here]**
*Show `top`, `free -h`, `ps` outputs.*

---

### 3.8 **Clean Teardown**

**Description:** Containers stopped with no zombie processes.

📸 **[Insert Screenshot Here]**
*Show `ps aux | grep engine`.*

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

## 📌 Notes

* Replace `<Your Name>` and `<Your SRN>`
* Add screenshots in `/images` folder:

```md
![Example](images/screenshot1.png)
```

---
