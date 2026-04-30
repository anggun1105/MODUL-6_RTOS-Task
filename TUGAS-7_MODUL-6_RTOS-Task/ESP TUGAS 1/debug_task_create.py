#!/usr/bin/env python3
"""
Debug Serial Parser & Visualizer - ESP32_01_Task_Create_Basic
Mem-parse output [DATA] dari serial dan menampilkan visualisasi
pembuatan task, heap usage, dan aktivitas task.

Usage:
    python debug_task_create.py --port /dev/ttyUSB0
    python debug_task_create.py --file capture.log
"""

import sys
import argparse
import time
import re
from collections import defaultdict
from datetime import datetime

try:
    import serial
except ImportError:
    serial = None

try:
    import matplotlib
    matplotlib.use('TkAgg')
    import matplotlib.pyplot as plt
    import matplotlib.animation as animation
    from matplotlib.gridspec import GridSpec
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib tidak tersedia. Install: pip install matplotlib")


class TaskCreateParser:
    """Parser untuk data serial ESP32 Task Create Basic"""

    def __init__(self):
        self.task1_data = []       # (timestamp_ms, led_state, count, core)
        self.task2_data = []
        self.heap_data = []        # (timestamp_ms, free_heap, min_heap)
        self.system_data = []      # (timestamp_ms, num_tasks, t1_count, t2_count)
        self.task_info = []        # (timestamp_ms, name, priority, stack_hwm, state, core)
        self.create_events = []    # (timestamp_ms, name, heap_after, heap_used)
        self.init_info = {}
        self.start_time = time.time()

    def parse_line(self, line):
        """Parse satu baris output serial"""
        line = line.strip()
        if not line.startswith("[DATA]"):
            return

        parts = line.replace("[DATA] ", "").split(",")
        if len(parts) < 2:
            return

        tag = parts[0]
        try:
            if tag == "INIT" and len(parts) >= 4:
                self.init_info = {
                    'timestamp': int(parts[1]),
                    'cores': int(parts[2]),
                    'heap': int(parts[3])
                }
                print(f"[INIT] Cores={parts[2]}, Heap={parts[3]} bytes")

            elif tag == "TASK1" and len(parts) >= 5:
                ts, led, count, core = int(parts[1]), int(parts[2]), int(parts[3]), int(parts[4])
                self.task1_data.append((ts, led, count, core))

            elif tag == "TASK2" and len(parts) >= 5:
                ts, led, count, core = int(parts[1]), int(parts[2]), int(parts[3]), int(parts[4])
                self.task2_data.append((ts, led, count, core))

            elif tag == "HEAP" and len(parts) >= 4:
                ts, free_h, min_h = int(parts[1]), int(parts[2]), int(parts[3])
                self.heap_data.append((ts, free_h, min_h))

            elif tag == "SYSTEM" and len(parts) >= 5:
                ts = int(parts[1])
                num_tasks, t1c, t2c = int(parts[2]), int(parts[3]), int(parts[4])
                self.system_data.append((ts, num_tasks, t1c, t2c))

            elif tag == "TASK_INFO" and len(parts) >= 7:
                name = parts[1]
                ts = int(parts[2])
                prio = int(parts[3])
                stack = int(parts[4])
                state = parts[5]
                core = int(parts[6])
                self.task_info.append((ts, name, prio, stack, state, core))

            elif tag == "CREATE" and len(parts) >= 5:
                name = parts[1]
                ts = int(parts[2])
                heap_after = int(parts[3])
                heap_used = int(parts[4])
                self.create_events.append((ts, name, heap_after, heap_used))
                print(f"[CREATE] {name}: heap_after={heap_after}, used={heap_used}")

        except (ValueError, IndexError) as e:
            pass

    def print_summary(self):
        """Cetak ringkasan data yang dikumpulkan"""
        print("\n" + "=" * 60)
        print("RINGKASAN DATA TASK CREATE BASIC")
        print("=" * 60)
        print(f"Task1 samples: {len(self.task1_data)}")
        print(f"Task2 samples: {len(self.task2_data)}")
        print(f"Heap samples:  {len(self.heap_data)}")
        print(f"System samples: {len(self.system_data)}")
        print(f"Task info samples: {len(self.task_info)}")
        print(f"Create events: {len(self.create_events)}")

        if self.heap_data:
            heaps = [h[1] for h in self.heap_data]
            print(f"\nHeap: min={min(heaps)}, max={max(heaps)}, "
                  f"last={heaps[-1]}")

        if self.task1_data:
            cores_t1 = [d[3] for d in self.task1_data]
            print(f"\nTask1 cores used: {set(cores_t1)}")

        if self.task2_data:
            cores_t2 = [d[3] for d in self.task2_data]
            print(f"Task2 cores used: {set(cores_t2)}")

        print("=" * 60)

    def plot_results(self):
        """Buat grafik visualisasi"""
        if not HAS_MATPLOTLIB:
            print("[WARN] Matplotlib tidak tersedia, skip plotting")
            return

        fig = plt.figure(figsize=(14, 10))
        fig.suptitle("ESP32 FreeRTOS Task Create Basic - Analysis",
                     fontsize=14, fontweight='bold')
        gs = GridSpec(3, 2, figure=fig, hspace=0.4, wspace=0.3)

        # 1. Task Activity Timeline
        ax1 = fig.add_subplot(gs[0, :])
        if self.task1_data:
            t1_ts = [d[0] / 1000.0 for d in self.task1_data]
            t1_led = [d[1] for d in self.task1_data]
            ax1.step(t1_ts, t1_led, where='post', label='Task1 LED1', color='blue', alpha=0.7)
        if self.task2_data:
            t2_ts = [d[0] / 1000.0 for d in self.task2_data]
            t2_led = [d[1] + 2 for d in self.task2_data]
            ax1.step(t2_ts, t2_led, where='post', label='Task2 LED2', color='red', alpha=0.7)
        ax1.set_xlabel("Waktu (detik)")
        ax1.set_ylabel("LED State")
        ax1.set_title("Task Activity Timeline")
        ax1.legend()
        ax1.grid(True, alpha=0.3)

        # 2. Heap Usage
        ax2 = fig.add_subplot(gs[1, 0])
        if self.heap_data:
            h_ts = [d[0] / 1000.0 for d in self.heap_data]
            h_free = [d[1] / 1024.0 for d in self.heap_data]
            h_min = [d[2] / 1024.0 for d in self.heap_data]
            ax2.plot(h_ts, h_free, 'g-', label='Free Heap', linewidth=2)
            ax2.plot(h_ts, h_min, 'r--', label='Min Free', linewidth=1)
            ax2.fill_between(h_ts, h_min, h_free, alpha=0.2, color='green')
        ax2.set_xlabel("Waktu (detik)")
        ax2.set_ylabel("Heap (KB)")
        ax2.set_title("Heap Memory Usage")
        ax2.legend()
        ax2.grid(True, alpha=0.3)

        # 3. Task Count Progress
        ax3 = fig.add_subplot(gs[1, 1])
        if self.system_data:
            s_ts = [d[0] / 1000.0 for d in self.system_data]
            s_t1 = [d[2] for d in self.system_data]
            s_t2 = [d[3] for d in self.system_data]
            ax3.plot(s_ts, s_t1, 'b-o', label='Task1 Count', markersize=3)
            ax3.plot(s_ts, s_t2, 'r-s', label='Task2 Count', markersize=3)
        ax3.set_xlabel("Waktu (detik)")
        ax3.set_ylabel("Iteration Count")
        ax3.set_title("Task Iteration Progress")
        ax3.legend()
        ax3.grid(True, alpha=0.3)

        # 4. Core Assignment
        ax4 = fig.add_subplot(gs[2, 0])
        if self.task1_data and self.task2_data:
            cores_t1 = [d[3] for d in self.task1_data]
            cores_t2 = [d[3] for d in self.task2_data]
            labels = ['Core 0', 'Core 1']
            t1_cores = [cores_t1.count(0), cores_t1.count(1)]
            t2_cores = [cores_t2.count(0), cores_t2.count(1)]
            x = range(len(labels))
            width = 0.35
            ax4.bar([i - width/2 for i in x], t1_cores, width, label='Task1', color='blue')
            ax4.bar([i + width/2 for i in x], t2_cores, width, label='Task2', color='red')
            ax4.set_xticks(list(x))
            ax4.set_xticklabels(labels)
        ax4.set_ylabel("Jumlah Eksekusi")
        ax4.set_title("Core Assignment Distribution")
        ax4.legend()
        ax4.grid(True, alpha=0.3, axis='y')

        # 5. Task Create Memory Cost
        ax5 = fig.add_subplot(gs[2, 1])
        if self.create_events:
            names = [e[1] for e in self.create_events]
            costs = [e[3] / 1024.0 for e in self.create_events]
            colors = ['#2196F3', '#4CAF50', '#FF9800', '#F44336']
            ax5.bar(names, costs, color=colors[:len(names)])
            for i, (n, c) in enumerate(zip(names, costs)):
                ax5.text(i, c + 0.1, f'{c:.1f}KB', ha='center', fontsize=9)
        ax5.set_ylabel("Memory Cost (KB)")
        ax5.set_title("Task Creation Memory Cost")
        ax5.grid(True, alpha=0.3, axis='y')

        plt.savefig("debug_task_create.png", dpi=150, bbox_inches='tight')
        print("[INFO] Grafik disimpan: debug_task_create.png")
        plt.show()


def read_serial(port, baudrate, parser, duration=60):
    """Baca data dari serial port"""
    if serial is None:
        print("[ERROR] pyserial tidak terinstall. Install: pip install pyserial")
        return

    print(f"[INFO] Membuka {port} @ {baudrate} baud...")
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"[INFO] Connected! Mengumpulkan data selama {duration} detik...")
        start = time.time()
        while (time.time() - start) < duration:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore')
                print(line.rstrip())
                parser.parse_line(line)
        ser.close()
    except serial.SerialException as e:
        print(f"[ERROR] Serial error: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh user")
        if 'ser' in locals():
            ser.close()


def read_file(filepath, parser):
    """Baca data dari file log"""
    print(f"[INFO] Membaca file: {filepath}")
    try:
        with open(filepath, 'r') as f:
            for line in f:
                parser.parse_line(line)
        print(f"[INFO] Selesai membaca file")
    except FileNotFoundError:
        print(f"[ERROR] File tidak ditemukan: {filepath}")
    except Exception as e:
        print(f"[ERROR] Error membaca file: {e}")


def main():
    argp = argparse.ArgumentParser(
        description="Debug parser untuk ESP32 Task Create Basic")
    argp.add_argument('--port', type=str, default=None,
                      help='Serial port (e.g., /dev/ttyUSB0)')
    argp.add_argument('--baud', type=int, default=115200,
                      help='Baud rate (default: 115200)')
    argp.add_argument('--file', type=str, default=None,
                      help='File log untuk di-parse')
    argp.add_argument('--duration', type=int, default=60,
                      help='Durasi capture dalam detik (default: 60)')
    args = argp.parse_args()

    parser = TaskCreateParser()

    if args.file:
        read_file(args.file, parser)
    elif args.port:
        read_serial(args.port, args.baud, parser, args.duration)
    else:
        print("[INFO] Mode demo - membaca dari stdin (Ctrl+C untuk berhenti)")
        try:
            for line in sys.stdin:
                print(line.rstrip())
                parser.parse_line(line)
        except KeyboardInterrupt:
            pass

    parser.print_summary()
    parser.plot_results()


if __name__ == "__main__":
    main()
