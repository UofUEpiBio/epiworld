---
title: Epiworld
hide:
  - navigation
  - toc
---

<div class="epiworld-hero epiworld-network-hero">
  <canvas class="epiworld-network-canvas" aria-hidden="true"></canvas>

  <div class="hero-text">
    <h1><img src="assets/branding/epiworld.png" width="200px"></h1>

    <p class="hero-tagline">
      A fast, general-purpose C++ framework for agent-based epidemiological simulation.
    </p>

    <div class="hero-badges">
      <span class="hero-badge">⚡ 150M+ agent·day/sec</span>
      <span class="hero-badge">📦 Header-only</span>
      <span class="hero-badge">🧬 Multi-pathogen</span>
      <span class="hero-badge">🌐 Network-aware</span>
    </div>

    <div class="hero-buttons">
      <a href="impl/" class="btn-primary">Get Started</a>
      <a href="https://github.com/UofUEpiBio/epiworld" class="btn-secondary">GitHub →</a>
    </div>
  </div>
</div>

<div class="lang-logos" markdown>
  <div class="lang-logos-row">
    <a href="https://github.com/UofUEpiBio/epiworld" class="lang-logo-card" aria-label="C++">
      <img src="assets/img/cpp-logo.svg" alt="C++ logo" style="width:100px">
      <p class="lang-logo-title">C++</p>
      <p class="lang-logo-text">Freely available on GitHub, including the single header.</p>
    </a>
    <a href="https://github.com/UofUEpiBio/epiworldR" class="lang-logo-card" aria-label="R">
      <img src="assets/img/r-logo.svg" alt="R logo" style="width:100px">
      <p class="lang-logo-title">R</p>
      <p class="lang-logo-text">Available both on GitHub and the Comprehensive R Archive Network (CRAN).</p>
    </a>
  </div>
</div>

## Try It Live

Edit the code below and click **▶ Run** to compile and execute it in your browser via [Compiler Explorer](https://godbolt.org). No local setup needed!

<div class="epiworld-playground" id="helloworld-playground">
  <div class="playground-toolbar">
    <span class="playground-label">🧪 Interactive Example</span>
    <span class="playground-status"></span>
    <button class="playground-btn playground-reset">↺ Reset</button>
    <button class="playground-btn playground-run">▶ Run</button>
  </div>
  <textarea class="playground-editor" spellcheck="false">
#include "epiworld.hpp"

using namespace epiworld;

int main() {

    // epiworld already comes with a couple
    // of models, like the SIR
    epimodels::ModelSIR<> hello(
        "COVID-19", // Name of the virus
        0.01,       // Initial prevalence
        0.1,        // Transmission probability
        0.3         // Recovery probability
        );

    // We can simulate agents using a smallworld network
    // with 10,000 individuals, in this case
    hello.agents_smallworld(10000, 10, false, 0.01);

    // Running the model and printing the results
    // Setting the number of days (100) and seed (122)
    hello.run(100, 122);
    hello.print();

    return 0;

}
</textarea>
  <div class="playground-output" style="display:none;">
    <div class="playground-output-header">Output</div>
    <pre class="playground-output-content"></pre>
  </div>
</div>

<div class="feature-grid" markdown>

<div class="feature-card" markdown>
<div class="feature-icon">⚡</div>

### Lightning Fast

Over 150 million agent × day simulations per second — built for
large-scale studies without sacrificing speed.
</div>

<div class="feature-card" markdown>
<div class="feature-icon">🧩</div>

### Fully Flexible

Define arbitrary states, viruses, tools, and update rules. Build
the exact model you need from composable primitives.
</div>

<div class="feature-card" markdown>
<div class="feature-icon">📦</div>

### Header-Only

Single-file include with zero external dependencies — just the
C++ standard library. Drop it in and go.
</div>

<div class="feature-card" markdown>
<div class="feature-icon">🌐</div>

### Network-Aware

Simulations run on contact networks with configurable topologies
including small-world, scale-free, and custom graphs.
</div>

<div class="feature-card" markdown>
<div class="feature-icon">🧬</div>

### Multi-Pathogen

Multiple viruses and tools can coexist in one simulation with
independent transmission and recovery dynamics.
</div>

<div class="feature-card" markdown>
<div class="feature-icon">📊</div>

### Rich Analytics

Built-in data collection, reproductive number tracking, generation
intervals, and likelihood-free inference (LFMCMC).
</div>

</div>

<div class="network-banner">
  <img src="assets/img/network-2.svg" alt="Epiworld network visualization">
</div>

## Documentation Sections

<div class="grid cards" markdown>

-   :material-book-open-variant:{ .lg .middle } **Models**

    ---

    Browse all built-in compartmental models: SIR, SEIR, SIS, connected
    population variants, mixing models, and disease-specific models.

    [:octicons-arrow-right-24: View models](epiworld/modules.md)

-   :material-code-tags:{ .lg .middle } **API Reference**

    ---

    Complete class and function reference generated from the source code
    via Doxygen.

    [:octicons-arrow-right-24: API docs](api/index.md)

-   :material-cog:{ .lg .middle } **Implementation**

    ---

    Deep dives into library architecture, performance optimization,
    extending the library, queueing system, and more.

    [:octicons-arrow-right-24: Learn more](impl/index.md)

</div>

## Source Code

The epiworld source code is hosted at
[**UofUEpiBio/epiworld**](https://github.com/UofUEpiBio/epiworld) on GitHub.
