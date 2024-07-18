## Test command

```bash
time ./main hollywood.jpg 15 kernel.tif deblurred.png --seed=1234
```

## Expected result

`refactored_code/ref_deblurred.png`

## Performance 

| Case | Time, s |
|----|---|
| Base code | 12.846 |
| Clang (no OMP) | 36.516 |
| GCC (no OMP) | 36.109 |
| GCC | 11.556 |
| GCC (-O2) | 12.527 |
| GCC (-O1) | 12.284 |
| GCC (-O0) | 24.458 |
| GCC (Debug) | 24.402 |
| GCC (Release) | 11.755 |

## Notes
* The `-march=native` and `-mtune=native` do not seem to have effect on performance.
* The `-O3` and `Release` configuration show best performance