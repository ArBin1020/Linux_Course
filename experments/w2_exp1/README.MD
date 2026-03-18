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
    - cache-references: 所有 LLC 存取次數
    - cache-misses: LLC miss 次數
    - L1-dcache-load-misses: L1 data cache 載入未命中次數
    - cycles: 處理器執行的總時脈週期
    - instructions: 總指令數
    - dTLB-load-misses: Data TLB 發生未命中的次數
    
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

單獨編譯與執行測試的方法
```bash
gcc -O2 -I../../include perf_benchmark.c ../../src/list.c ../../src/array.c -o perf_runner

./perf_runner <exp> <N> <mode> <reps>
# exp: 1 (Allocation Strategy) 或 2 (Spatial Locality Scan)。
# N: 節點總數量
# mode: 依據 exp 選擇 0 或 1（定義如上）
# reps: 迴圈重複次數。用於放大 perf 事件計數

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

-X 軸:
    資料規模 $N$
    
- Y 軸 (正規化)：
    - Cycles / element: 完成單一元素操作所需耗費的硬體時脈週期
    - Instructions / element: 每次元素操作的軟體指令數
    - IPC (Instr. per Cycle): 每週期執行指令數
    - LLC references / element: 每元素觸發的 LLC 存取請求次數
    - LLC misses / element: 每元素必須向 DRAM 讀取資料的絕對次數
    - L1-dcache misses / element: L1 快取未命中次數
    - dTLB misses / element: 發生 Page Table Walk 的次數
    - LLC miss rate (%): 公式為 $LLC\_miss\_rate\_pct = \left( \frac{cache\_misses}{cache\_references} \right) \times 100$

### 直接使用
```bash
make
sudo bash perf_run.sh
python perf_plot.py
```

