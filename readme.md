# concurrent patterns

---

### Seqlock

seqlocks are more efficient than traditional read-write locks for the situation where there are many readers and few writers

reader never blocks, they use optimistic lock, if sequence not changed, then read success

writers must coordinate with mutex, it first advance sequence, then do write action, then advance sequence again

reference:
https://en.wikipedia.org/wiki/Seqlock
