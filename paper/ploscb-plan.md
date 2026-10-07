# Plan: epiworld software paper for PLOS Computational Biology

> Working plan (draft, 2026-10-07). Paths prefixed with `epiworld-benchmark:` refer to [UofUEpiBio/epiworld-benchmark](https://github.com/UofUEpiBio/epiworld-benchmark); other paths are relative to this repository.

## Context

The aim is to submit epiworld to *PLOS Computational Biology* as a **Software** article. Its frame is a fast, flexible, multi-language epidemic ABM engine, and it would report what is new since the epiworldR JOSS paper (Meyer & Vega Yon 2023, JOSS 8(90):5781, Oct 2023). The benchmark repository's issue UofUEpiBio/epiworld-benchmark#10 currently plans a *neutral* multi-engine benchmark paper for simulation journals.

The user has chosen three things:

1. **Two papers.** The software paper carries a compact benchmark subset and cites a separate, neutral benchmark paper or preprint (issue UofUEpiBio/epiworld-benchmark#10).
2. **Subject = the epiworld family.** The C++ core is the subject, with R, Python and JS/WASM bindings and the `measles` add-on. The C++ core has never been peer-reviewed; the JOSS paper covered only the R wrapper and is cited as prior work.
3. **Applied anchors:** Utah measles (with UDHHS), other jurisdictions (MN template, flepimop2/ACCIDDA), plus a new worked case study.

### PLOS CB constraints that shape everything

- The article is under 3,500 words, with fixed sections: Introduction, Design and Implementation, Results, Availability and Future Directions.
- "Enhancements to existing published open-source software will only be considered if those enhancements bring exceptional new capabilities." It "must already be widely adopted, or have the promise of wide adoption."
- The software must give a "significant advance in providing new biological insights" (in the paper or published elsewhere).
- Code, docs and test data must be deposited, and reviewers must be able to reproduce the results.

### Pushback / risks to address up front

- **The benchmark has a conflict of interest.** Several epiworld speedups were made *while* benchmarking against the other engines:
  - #266: push transmission
  - #275: counting-sort edge lists
  - #279: `ParamRef`
  - #281: push/pull fix

  Reviewers will see this as tuning your own engine against the test. To handle it:
  - Disclose it.
  - Freeze the versions.
  - Invite the other maintainers to review their runners (already in UofUEpiBio/epiworld-benchmark#10).
  - Keep the full comparison in the neutral paper.
- **"Fastest" depends on the metric.** The headline metric, simulation time, excludes model build, which epiworld does once and reuses. The software paper must also report total time per replicate (build + simulate) and memory. Otherwise the speed claim looks cherry-picked.
- **Speed alone does not satisfy PLOS CB.** The case for publication has to rest on capability plus applied impact (measles response work), with speed as supporting evidence.
- **"Multi-language" needs a sharper claim.** Other engines also have multiple front-ends. epiworld's distinctive points:
  - One header-only C++ core.
  - Wrappers with no measurable runtime cost (`epiworld-benchmark:docs/results.md`, "The language layer": wrappers are within 6% of C++).
  - Runs client-side in the browser via WASM.
  - Ideally, **identical output from the same seed across C++, R, Python and JS** (verify; see D4).
- **Fix housekeeping before citing:**
  - epiworld `CITATION.cff` says 0.10.0, but the code is at 0.18.0.
  - The README says C++11, but the build uses C++17.
  - The epiworldR NEWS has no dates.

## Inventory already gathered (basis for the "what's new" section)

**epiworld C++.**
- At the JOSS snapshot `7521a98` (2023-10-13): unversioned, 12 test files, ~18k header lines.
- Now: v0.18.0 (2026-10-07), with 120 test files / ~863 assertions and ~30k lines. There have been 316 commits since 2023-06.
- New capability clusters since JOSS:
  1. **Population structure.** Mixing models with contact matrices (`ModelSIRMixing`/`SEIRMixing`/`SEIRMixingQuarantine`, `ContactMatrix`, `sampler::Mixing`). Entity rework and `distribute_*_to_entities`. SBM network generator. Edge add/remove. Directed networks.
  2. **Interventions.** `ToolVaccine`, `ContactTracing` (all models), `QuarantineTrigger`, `Bubbles`, `ModelSEIRNetworkQuarantine`. In the measles library: PEP and immunoglobulin.
  3. **Measles library** (`include/measles`, v0.4.0, a separate R package `measles`): School, Mixing and MixingRiskQuarantine models, built with Utah DHHS.
  4. **Calibration.** LFMCMC overhaul (late 2024), plus the BiLSTM calibration package `epiworldRcalibrate` (arXiv:2509.07013).
  5. **Outputs.** Transmissions always recorded, generation-interval densities, `HospitalizationsTracker`, outbreak-size getters, contact callbacks, in-memory `run_multiple` outputs (`SaverMemory`), Mermaid/DOT model diagrams.
  6. **Model builder** in R (`Model()`, `add_state()`, …).
  7. **Performance and correctness.** Push/pull adaptive transmission, xoshiro256**, Poisson/Lemire sampling, `ParamRef`, ASan/thread-race fixes, cachegrind-backed docs (`docs/impl/transmission-sampling.md`).
- **The ecosystem is all new since JOSS, except epiworldRShiny (Sep 2023):**
  - epiworldpy: PyPI, 2025.
  - epiworldjs: npm/WASM, Oct 2026.
  - epiworldRShiny: CRAN, deployed, 49k schools in 25 states.
  - epiworld-docs (MkDocs).
  - epiworld-forecasts (weekly Utah COVID).
  - flepimop2-epiworld (ACCIDDA).
  - MN-measles-simulation.

## Work plan

### Phase A: Repository and framing (week 1)

1. Keep the manuscript under `paper/` in this repository, or move it to a dedicated repo (e.g., `UofUEpiBio/epiworld-ploscb`): a Quarto manuscript with `paper/`, `data-collection/` and `case-study/`.
   - Either way, keep it out of the benchmark repo so the benchmark stays neutral.
   - Cite the benchmark repo's tagged release for the benchmark subset.
2. Retarget issue UofUEpiBio/epiworld-benchmark#10's text to "neutral benchmark paper" and link to a new tracking issue in `UofUEpiBio/epiworld` for the software paper. Find and update the existing tracking issue on this.
3. Write a one-page **claims list**: each claim the paper makes, mapped to the evidence (D1–D7) that supports it. Everything else follows from it.

### Phase B: Data collection (scripted, in `data-collection/`)

| ID | What | How / source | Output |
|---|---|---|---|
| D1 | **Feature timeline since JOSS** | Script over `git tag --sort=creatordate` + release notes in epiworld, epiworldR, epiworldpy; hand-curated feature→PR→date mapping from the inventory above | `features.csv`; a timeline figure (versions × capability clusters) and a "then vs now" table (models 13→17+measles, tests 12→120, languages 1→4) |
| D2 | **Adoption metrics** (for the "wide adoption" criterion) | CRAN downloads (`cranlogs`) for epiworldR, epiworldRShiny, measles; PyPI (`pypistats`); npm downloads; GitHub stars/forks/contributors; reverse dependencies; JOSS-paper citations (OpenAlex/Google Scholar); Shiny app usage (Google Analytics, if available) | `adoption.csv` and one small figure or table |
| D3 | **Applied-use dossier** | Collect for each deployment what decision it informed, with dates, partners and a public artifact. Items: Utah DHHS measles (Short Creek, school scenarios), the MN template, the Utah COVID forecasts, flepimop2. **Get written OK from UDHHS for what can be named and shown, and consider UDHHS co-authors.** | `applications.md`; one Results subsection + Fig. |
| D4 | **Cross-language reproducibility** | Run the same model and seed in C++, R, Python and JS (Node) and compare outputs exactly (history, transmissions) | Table: identical (yes/no) per language pair. If not identical, find out why and fix or document it. A strong, unique claim. |
| D5 | **Compact performance evidence** | Reuse `epiworld-benchmark:results/results.csv` and its `analysis/` code, with no new engines. Subset: S00 at 10k/100k, S03 at 1M (scaling), S04 GeoPops; **simulation *and* total time**, peak memory, the language-layer ratio table. Add epiworld-only items: `run_multiple` OpenMP thread scaling, and the 0.15→0.18 version-over-version speedup | 1 multi-panel figure, 1 table |
| D6 | **Expressiveness comparison** | Reuse the feature summary in `epiworld-benchmark:docs/engines.md` and the scenario 05 "mixing and quarantine" design in `epiworld-benchmark:docs/showcase-models.md`; verify each engine's multi-disease support (Starsim connectors, FRED conditions, Covasim variants-only, ixa/individual/Agents.jl user code) from their docs | One feature-matrix table (multiple diseases + interaction mechanism, mixing, quarantine/isolation, contact tracing, resource/capacity feedback, calibration, language bindings, browser, transmission tree) |
| D7 | **Quality metrics** | Codecov %, CI matrix, test counts, docs pages, ASan/UBSan status | One sentence + table row in Availability |

### Phase C: New worked case study (`case-study/`): a winter season under hospital capacity, with a measles shock

**Question.** Over one winter, influenza, RSV and SARS-CoV-2 share one county's hospital beds. A measles outbreak is introduced mid-season.
- How much does each disease contribute to bed occupancy?
- When and for how long is capacity exceeded?
- How much does timing matter: a measles outbreak at the respiratory peak versus off-peak?
- Which levers keep capacity below the threshold (flu or COVID vaccination coverage, measles quarantine or PEP), and by how much?

Data are public only, so the study is fully reproducible.

**Model design** (constrained by what epiworld does; confirmed in the code):
- **No coinfection.** Each agent holds at most one virus (`agent-bones.hpp:220`, `VirusPtr virus`; multi-virus agents were removed in `a49f863`, 2023-10). The case study models an agent that is *infected with one virus and therefore not infectable by another* (an exclusion assumption), and the paper must state that.
- **Disease-specific immunity.** After recovery an agent returns to a susceptible state and receives a per-virus immunity `Tool`, whose susceptibility reduction depends on which virus is attempting infection. Waning comes from the tool's own decay or a global event.
- **Disease-specific states and branches:** E → I → (H) → R per virus, with hospitalization probabilities and lengths of stay by age group. `HospitalizationsTracker` gives bed occupancy.
- **Capacity feedback:** a `GlobalEvent` reads current hospitalizations each day. Above the bed threshold, it raises the risk of an adverse outcome, or diverts patients and lowers the probability of admission.
- **Seasonality:** a `GlobalEvent` that sets a time-varying transmission parameter for each respiratory virus (`globalevent_set_param` or a custom event).
- **Population:** the GeoPops synthetic population (Spartanburg; already used in S04 and the docs) with `ContactMatrix` school/household mixing. Measles parameters, quarantine and PEP mirror `ModelMeaslesMixingRiskQuarantine` (re-expressed in the multi-virus model).
- **Parameters:** from published sources, cited in a supplementary table: disease durations, age-specific hospitalization, length of stay, R0, VE and coverage. Seasonal amplitude/timing is calibrated with **LFMCMC** to public hospital-admission curves for flu, RSV and COVID (CDC NHSN/RESP-NET for one state or season; check licensing). Calibration is shown end to end; measles needs none.

**Feasibility spike first (1–2 weeks, before committing the paper to this):**
1. Build a minimal 2-virus model (flu + RSV) in C++ and in epiworldR with per-virus immunity tools, and check it against a hand-built expectation: (a) disease-specific reinfection works; (b) histories by virus are recorded separately (`get_hist_virus`, transmissions by virus); (c) the push/pull transmission path handles multiple viruses correctly.
2. Add the capacity `GlobalEvent` and seasonality, and check runtime at about 165k agents (one season = 210 days × reps).
3. If (c) or (b) fails, fix it in epiworld (that is a legitimate new capability for the paper), or fall back to the measles-only case study.

**Runs and outputs:**
- The main analysis is in R.
- Reproduce one panel in Python and in the browser (epiworldjs) to show the multi-language claim.
- Fig A: stacked bed occupancy by disease, with the capacity line and the measles shock at two timings.
- Fig B: intervention-response surface (respiratory vaccination × measles quarantine/PEP) against days over capacity.

**Framing caveats for the paper:**
- **Multiple diseases are not unique to epiworld.** Starsim is built around multi-disease models with "connectors", FRED supports multiple conditions, and ixa and Agents.jl can express it in user code. The claim is not "only epiworld can". It is "epiworld expresses this compactly with built-in pieces (tools, global events, mixing, LFMCMC), runs it fast, and runs it identically from R/Python/browser."
- **Exclusion is an assumption.** Single infection per agent means competition is only through exclusion, shared beds and behavior. That is a defensible approximation over one season, but say so.
- **Neutral benchmark:** a reduced version (two viruses + capacity, no calibration) is a natural candidate for the epiworld family's showcase in `epiworld-benchmark:docs/showcase-models.md` (it currently plans §05, mixing + quarantine). There it would be measured against Starsim on its home ground. Add it there only through the neutral-benchmark process (UofUEpiBio/epiworld-benchmark#24).

### Phase D: Manuscript (≤3,500 words)

| Section | ~Words | Content |
|---|---|---|
| Abstract + Author summary | 250 + 200 | — |
| Introduction | 500 | Gap: public-health ABMs need speed, structure (mixing/networks), interventions, calibration, and accessibility from R/Py/web; positioning against Covasim/Starsim, ixa, individual, FRED, Agents.jl; prior work (JOSS) |
| Design and Implementation | 1,000 | Header-only core; state/virus/tool/entity abstractions; push/pull transmission; mixing sampler; global events; LFMCMC; bindings architecture (Rcpp / pybind11 / Emscripten); testing and CI. Fig 1: architecture diagram |
| Results | 1,300 | (i) what's new (D1 table/timeline); (ii) performance (D5, citing the neutral benchmark); (iii) cross-language identity (D4); (iv) case study (Phase C); (v) deployment (D3) |
| Availability and Future Directions | 300 | Licenses, CRAN/PyPI/npm, Zenodo DOIs, docs; roadmap |

- **Supplementary material:** full feature table, the D6 matrix, the benchmark methods summary, adoption numbers, and the code for every figure.
- **Cover letter:** say explicitly why this is *not* an incremental update to the JOSS paper. The C++ core and three of four language bindings have never been peer-reviewed. Then list the exceptional new capabilities (clusters 1–5) and the public-health deployment.
- **Conflict of interest:** state that the authors develop epiworld and that the benchmark is run by the same group, and point to the neutral benchmark repo.

### Phase E: Release and deposit

- Fix `CITATION.cff` and the C++-standard note.
- Tag synchronized releases (epiworld, epiworldR on CRAN, epiworldpy, epiworldjs, measles) and get Zenodo DOIs.
- Deposit a `.7z` (<100 MB) with source, docs and case-study data + scripts.
- Provide a container (reuse the benchmark repo's container approach, `epiworld-benchmark:setup.md`) so reviewers can rerun D4, D5 and Phase C with one command.

## Order and dependencies

A → (D1, D2, D3, D7 in parallel; quick) and **the Phase C feasibility spike** → D4 (may surface bugs; start early) → D5 (mostly reuse) → the rest of Phase C → draft → Phase E.

The neutral benchmark paper (issue UofUEpiBio/epiworld-benchmark#10) proceeds in parallel. At minimum, post a preprint before the software paper is submitted so the software paper can cite it.

## Critical files and code to reuse

- `epiworld-benchmark:docs/results.md`: "The language layer" ratio table and the scaling numbers. `epiworld-benchmark:results/results.csv` and `epiworld-benchmark:analysis/` hold the figure code.
- `epiworld-benchmark:docs/engines.md` (feature summary) and `epiworld-benchmark:docs/showcase-models.md` (§05, epiworld mixing + quarantine) for D6 and the case-study design.
- `epiworld-benchmark:scenario_04/` for the GeoPops network loading.
- `docs/impl/transmission-sampling.md` and `docs/impl/performance-optimization.md` (this repo) for the performance narrative.
- epiworldR's `paper.md` (JOSS baseline: 13 models, features list).
- epiworldR's `NEWS.md` and this repo's release notes (`gh release list`).

## Verification

- Each D-script runs from a clean checkout with one `make` target and writes its CSV and figure. Rerunning gives the same numbers, except D2, which is dated and cached.
- D4 passes: same seed gives identical histories across all four languages, checked automatically in CI.
- The case study and figures rebuild inside the deposited container on a second machine.
- A word count of the Quarto render is ≤3,500, excluding supplementary material.
- Every claim in the claims list (A3) cites a D-artifact; an internal review checks this before submission.
