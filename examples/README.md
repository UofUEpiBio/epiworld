# Some examples with `epiworld`

The file `simple-example.cpp` simulates COVID-19 Assuming people can wear mask, vaccinate, and
acquire an important immunity after recovery.

To compile

```bash
make
```

To run examples:

```bash
./simple-world.o [seed] [days] [initial prevalence] [mutation rate]
```

For example:

```
./simple-world.o 99 365 .01 .0000325
```


