## benchmark 說明
### 實驗一：比較大量的節點建立速度
- 比較 per-node malloc 跟 memory pool 在不同規模下的建構總耗時
- 比較平均單一節點耗時
- 量測：
    - total_ns   ：建構整條串列的絕對總耗時
    - avg_ns     ：平均每個節點耗時 (total / N)
### 實驗二：記憶體連續性對走訪速度的影響
- 目標
    - 比較循序配置 malloc linked list 跟 指標連結順序完全打亂的 linked list 
    - 比較在走訪時的平均單一節點耗時
- 具體內容
    採用了 Memory Pool 分配，確保兩種走訪方式的實體記憶體位置一致，將效能差異控制在只有 **存取順序的實體記憶體空間不同** 導致的速度不同上
### 圖表說明
- 圖表一(exp2_sub1_alloc)

- 圖表二(exp2_sub2_locality)
### 
Sequential Pool Scan  (pool, 0→1→2→… link order)
Shuffled Pool Scan  (same pool, randomised link order)
### usage
``` bash
make
./bench
python plot.py
```

## perf 
### perf_benchmark.c


- 觀測事件說明
    - cache-references      : 所有 LLC 存取次數（分母，用於計算 miss rate）
    - cache-misses          : LLC miss 次數（= 需要去 DRAM 取資料）
    - L1-dcache-load-misses : L1 data cache load miss（最敏感的近端指標）
    - dTLB-load-misses      : data TLB miss（存取分散位址時的頁表查詢懲罰）
    - instructions          : 總指令數（用於計算 MPKI = misses per kilo-inst）
1. Exp 1 記憶體配置策略
    測量建立Linked List 時的記憶體管理開銷
    - Mode 0
        針對 $N$ 個節點獨立呼叫 $N$ 次 malloc 與 free
    - Mode 1
        單次呼叫 malloc 取得 $N$ 個節點所需的連續實體記憶體塊
2. Exp 2：Spatial Locality Scan
    測量走訪Linked List 時，記憶體實體佈局對 CPU 快取架構的影響
    - Mode 0 (Sequential Pool Scan)
    - Mode 1 (Shuffled Pool Scan)
        使用 Fisher-Yates 演算法將節點索引打亂
- usage:
```bash

./perf_runner <exp> <N> <mode> <reps>
    # exp  = 1  Allocation Strategy
    #     mode 0 = per-node malloc  (每次 alloc 一個節點)
    #     mode 1 = memory pool      (單次 malloc 整塊記憶體)
    # exp  = 2  Spatial Locality Scan
    #     mode 0 = sequential pool  (next 指標循序連結 0→1→2→…)
    #     mode 1 = shuffled pool    (next 指標以 Fisher-Yates 打亂)
    # reps = 重複次數，放大 perf 事件計數，建議 Exp1 用 20，Exp2 用 50
```

單一使用
```bash
gcc -O2 -I../../include perf_benchmark.c ../../src/list.c ../../src/array.c -o perf_runner

perf stat -r 3 \
    -e instructions, cache-references, cache-misses, L1-dcache-load-misses, dTLB-load-misses \
    -- ./perf_runner 1 1000000 0 20
```

### run_perf.sh
perf 量測腳本
解析 perf stat 輸出並存為 CSV，供 plot_perf.py 繪圖使用
```bash
bash perf_run.sh [output.csv]
```

### plot_perf.py
將 run_perf.sh 輸出的 CSV 繪製成 perf 指標比較圖
用法：
```bash
python3 perf_plot.py                  # 讀取 perf_results.csv
python3 perf_plot.py other.csv        # 指定 CSV 路徑
```
輸出：
```bash
perf_exp1.png  — Exp1: Allocation Strategy
perf_exp2.png  — Exp2: Spatial Locality Scan
```
### 圖表說明
- X 軸
    - $10^4$
    - $10^6$
    - $10^8$

- Y 軸
    - instructions/element: 每次元素操作的指令數（反映演算法複雜度）
    - LLC references:       每次元素操作的 Last-Level Cache 存取次數
    - LLC misses:           需要到 DRAM 取資料的次數
    - L1-dcache misses:     L1 快取未命中
    - dTLB misses:          頁表查詢次數
    - LLC miss rate (%):    cache_misses / cache_references × 100

### usage
```bash
sudo bash perf_run.sh
python perf_plot.py
```

