## benchmark 說明
### 實驗一：Tail Append & System Overhead
觀察 linked list 走訪成本、malloc 及 Array
- 主要觀測幾個：
    1. LL Unknown Tail + Malloc：每次插入皆須從 Head 走訪至 Tail，且觸發 System Call。時間複雜度 $O(N)$。
    2. LL Known Tail + Malloc：維護 Tail 指標，直接在尾端插入，但保留 System Call 開銷。時間複雜度 $O(1)$。
    3. LL Unknown Tail + Pool：使用 Memory Pool 消除 malloc 開銷，但保留走訪成本。時間複雜度 $O(N)$。
    4. LL Known Tail + Pool：Linked List tail append，依靠指標操作且無 System Call。嚴格時間複雜度 $O(1)$。
    5. Array Append：依賴連續記憶體與動態擴容。均攤時間複雜度 $O(1)$

    NOTE: pool 是一開始就先 malloc 好，之後新增節點從 pool 取得

### 實驗二：最差情況插入與記憶體搬移
測量 Array 在非尾端插入時，因資料平移所造成的記憶體遷移 overhead
- 觀測以下行為：
    1. Array Front Insert：每次在索引 0 插入，強制呼叫 memmove 將現有 $N$ 個元素向後平移。
    2. LL Head Prepend (Pool)：在 linked list 頭部插入，僅需重定向指標。時間複雜度 $O(1)$。

### 實驗三：Tail Latency跟穩定性分析
單次插入操作的 Max Latency 與 p99 Latency
- 觀測
    1. Array Append (Max Latency)：當 size == capacity 時，觸發底層 Page Fault 與整塊記憶體複製的極端耗時情況
    2. LL Known Tail + Pool (Max Latency)：有 memory pool 的 Linked List，其每次插入延遲皆維持在極低且穩定的常數波動範圍內

### 實驗四：Sequential Scan 跟 Cache Locality
分析記憶體連續性跟 CPU Cache Miss 的影響
- 觀測以下行為：
    1. Array Scan：Spatial Locality，作為硬體讀取效能的 baseline
    2. Fragmented LL Scan：模擬節點散佈於 Heap 各處的 linked list ，看 Cache Miss 與 TLB Miss 帶來的延遲
    3. Continuous LL Scan：直接從 Pool 中循序初始化，節點在記憶體位址上是完全連續的，但仍透過 next 指標進行走訪。看 指標的 overhead 跟 記憶體不連續的 overhead

### usage
``` bash
make
./bench
python plot.py
```