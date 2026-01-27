# Some examples with `epiworld`

Each folder contains a C++ example with the corresponding output.

To build all examples, type from the root of the repository:

```bash
make examples
```

To build a specific example, use the full build path. For example, to build `10-likelihood-free-mcmc`:

```bash
make build/examples/10-likelihood-free-mcmc/10-likelihood-free-mcmc
```

Then run it:

```bash
./build/examples/10-likelihood-free-mcmc/10-likelihood-free-mcmc
```

More generally:

```bash
make build/examples/<example-name>/<example-name>
./build/examples/<example-name>/<example-name>
```