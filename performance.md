## Test command

```bash
time ./main hollywood.jpg 15 kernel.tif deblurred.png
```

## Performance 

| Case | Time, s |
|----|---|
| Base code | 12.846 |
| Clang (no OMP) | 36.516 |
| GCC (no OMP) | 36.109 |
| GCC | 11.556 |

## Notes
* The `-march=native` and `-mtune=native` do not seem to have effect on performance.