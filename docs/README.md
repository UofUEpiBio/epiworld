---
title: Epiworld
---

```{=html}
<div class="epiworld-hero epiworld-network-hero">
  <canvas class="epiworld-network-canvas" aria-hidden="true"></canvas>
  <div class="hero-text">
    <h1><img src="assets/branding/epiworld.png" width="200px" alt="Epiworld logo"></h1>
    <p class="hero-tagline">A fast, general-purpose C++ framework for agent-based epidemiological simulation.</p>
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

<div class="lang-logos">
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
```

## Try It Live

Edit the code below and click **▶ Run** to compile and execute it in your browser via [Compiler Explorer](https://godbolt.org). No local setup needed!

```{=html}
<div class="epiworld-playground" id="helloworld-playground">
  <div class="playground-toolbar">
    <span class="playground-label">🧪 Interactive Example</span>
    <span class="playground-status"></span>
    <button class="playground-btn playground-reset">↺ Reset</button>
    <button class="playground-btn playground-run">▶ Run</button>
  </div>
  <textarea class="playground-editor" spellcheck="false">#include "epiworld.hpp"

using namespace epiworld;

int main() {

    // epiworld already comes with a couple
    // of models, like the SIR
    epimodels::ModelSIR&lt;&gt; hello(
        "COVID-19", // Name of the virus
        0.01,       // Initial prevalence
        0.1,        // Transmission probability
        0.3         // Recovery probability
        );

    // We can simulate agents using a small-world network
    // with 10,000 individuals, in this case
    hello.agents_smallworld(10'000, 10, false, 0.01);

    // Running the model and printing the results
    // Setting the number of days (100) and seed (122)
    hello.run(100, 122);
    hello.print();

}</textarea>
  <div class="playground-output" style="display:none;">
    <div class="playground-output-header">Output</div>
    <pre class="playground-output-content"></pre>
  </div>
</div>
```

## Highlights

```{=html}
<div class="feature-grid">
  <div class="feature-card">
    <div class="feature-icon">⚡</div>
    <h3>Lightning Fast</h3>
    <p>Over 150 million agent × day simulations per second for large-scale studies.</p>
  </div>
  <div class="feature-card">
    <div class="feature-icon">🧩</div>
    <h3>Fully Flexible</h3>
    <p>Define arbitrary states, viruses, tools, and update rules from composable primitives.</p>
  </div>
  <div class="feature-card">
    <div class="feature-icon">📦</div>
    <h3>Header-Only</h3>
    <p>Single-file include with zero external dependencies beyond the C++ standard library.</p>
  </div>
  <div class="feature-card">
    <div class="feature-icon">🌐</div>
    <h3>Network-Aware</h3>
    <p>Run simulations on small-world, scale-free, and custom network topologies.</p>
  </div>
  <div class="feature-card">
    <div class="feature-icon">🧬</div>
    <h3>Multi-Pathogen</h3>
    <p>Model multiple viruses and tools in the same simulation with independent dynamics.</p>
  </div>
  <div class="feature-card">
    <div class="feature-icon">📊</div>
    <h3>Rich Analytics</h3>
    <p>Track generation intervals, reproductive numbers, and likelihood-free inference outputs.</p>
  </div>
</div>

<div class="network-banner">
  <img src="assets/img/network-2.svg" alt="Epiworld network visualization">
</div>
```

## Documentation Sections

```{=html}
<div class="docs-section-grid">
  <a class="docs-section-card" href="models/index.html">
    <span class="docs-section-kicker">Models</span>
    <h3>Built-in Models</h3>
    <p>Browse the major epidemiological model families bundled with the library.</p>
    <span class="docs-section-cta">View models →</span>
  </a>
  <a class="docs-section-card" href="api/index.html">
    <span class="docs-section-kicker">API Reference</span>
    <h3>Generated C++ Reference</h3>
    <p>Open the MrDocs-generated class, function, namespace, and symbol reference.</p>
    <span class="docs-section-cta">Explore API →</span>
  </a>
  <a class="docs-section-card" href="impl/index.html">
    <span class="docs-section-kicker">Implementation</span>
    <h3>Architecture Guides</h3>
    <p>Read about performance, queueing, random number generation, and extension points.</p>
    <span class="docs-section-cta">Learn more →</span>
  </a>
</div>
```

## Source Code

The epiworld source code is hosted at
[**UofUEpiBio/epiworld**](https://github.com/UofUEpiBio/epiworld) on GitHub.
