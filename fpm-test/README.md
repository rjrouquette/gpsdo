# Fixed-Point Math Test

Simple C++ program for testing temperature coefficient fixed point math.

## InfluxDB Export

Example data set exported as `gpsdo.csv`

Query:
```
SELECT mean("temperature"), mean("pll_ppm") FROM "gpsdo" WHERE  AND time >= now() - 120d GROUP BY time(11s) fill(null)
```
