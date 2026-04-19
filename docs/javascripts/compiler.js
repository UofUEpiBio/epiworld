(function () {
  "use strict";

  var HEADER_URL =
    "https://raw.githubusercontent.com/UofUEpiBio/epiworld/main/epiworld.hpp";
  var CE_API_BASE = "https://godbolt.org/api";
  var CE_COMPILER = "g132";
  var CE_FLAGS = "-std=c++17 -O2";

  var headerCache = null;

  function fetchHeader() {
    if (headerCache) {
      return Promise.resolve(headerCache);
    }
    return fetch(HEADER_URL)
      .then(function (response) {
        if (!response.ok) {
          throw new Error("Failed to fetch epiworld.hpp: " + response.status);
        }
        return response.text();
      })
      .then(function (text) {
        headerCache = text;
        return text;
      });
  }

  function inlineHeader(source, header) {
    return source.replace(
      /^\s*#include\s*["<]epiworld\.hpp[">]\s*$/m,
      header
    );
  }

  function compileAndRun(source) {
    return fetchHeader()
      .then(function (header) {
        var fullSource = inlineHeader(source, header);
        return fetch(CE_API_BASE + "/compiler/" + CE_COMPILER + "/compile", {
          method: "POST",
          headers: {
            "Content-Type": "application/json",
            Accept: "application/json",
          },
          body: JSON.stringify({
            source: fullSource,
            options: {
              userArguments: CE_FLAGS,
              executeParameters: { args: "", stdin: "" },
              compilerOptions: { executorRequest: true },
              filters: {
                execute: true,
              },
            },
          }),
        });
      })
      .then(function (response) {
        if (!response.ok) {
          throw new Error("Compiler Explorer returned " + response.status);
        }
        return response.json();
      })
      .then(function (data) {
        var output = "";
        var hasError = false;

        if (data.buildResult && data.buildResult.stderr) {
          var buildErrors = data.buildResult.stderr
            .map(function (l) {
              return l.text;
            })
            .join("\n")
            .trim();
          if (buildErrors) {
            output += buildErrors + "\n";
            hasError = true;
          }
        }

        if (data.stderr) {
          var stderr = data.stderr
            .map(function (l) {
              return l.text;
            })
            .join("\n")
            .trim();
          if (stderr) {
            output += stderr + "\n";
            hasError = true;
          }
        }

        if (data.stdout) {
          output += data.stdout
            .map(function (l) {
              return l.text;
            })
            .join("\n");
        }

        if (data.execResult) {
          if (data.execResult.buildResult && data.execResult.buildResult.stderr) {
            var execBuildErr = data.execResult.buildResult.stderr
              .map(function (l) {
                return l.text;
              })
              .join("\n")
              .trim();
            if (execBuildErr) {
              output += execBuildErr + "\n";
              hasError = true;
            }
          }
          if (data.execResult.stderr) {
            var execStderr = data.execResult.stderr
              .map(function (l) {
                return l.text;
              })
              .join("\n")
              .trim();
            if (execStderr) {
              output += execStderr + "\n";
            }
          }
          if (data.execResult.stdout) {
            output += data.execResult.stdout
              .map(function (l) {
                return l.text;
              })
              .join("\n");
          }
        }

        if (!output.trim()) {
          if (hasError) {
            output = "Compilation failed. Check errors above.";
          } else {
            output = "(no output)";
          }
        }

        return { output: output.trim(), hasError: hasError };
      });
  }

  function escapeHtml(text) {
    var div = document.createElement("div");
    div.appendChild(document.createTextNode(text));
    return div.innerHTML;
  }

  function initPlayground(container) {
    var editor = container.querySelector(".playground-editor");
    var runBtn = container.querySelector(".playground-run");
    var resetBtn = container.querySelector(".playground-reset");
    var outputWrap = container.querySelector(".playground-output");
    var outputContent = container.querySelector(".playground-output-content");
    var statusEl = container.querySelector(".playground-status");

    if (!editor || !runBtn || !outputWrap || !outputContent) {
      return;
    }

    var originalCode = editor.value;

    function setStatus(msg) {
      if (statusEl) {
        statusEl.textContent = msg;
      }
    }

    function setRunning(running) {
      runBtn.disabled = running;
      if (running) {
        runBtn.classList.add("is-loading");
        runBtn.textContent = "⏳ Running…";
      } else {
        runBtn.classList.remove("is-loading");
        runBtn.textContent = "▶ Run";
      }
    }

    runBtn.addEventListener("click", function () {
      var source = editor.value;
      outputWrap.style.display = "block";
      outputContent.textContent = "";
      outputContent.className = "playground-output-content";
      setRunning(true);
      setStatus("Fetching library & compiling…");

      compileAndRun(source)
        .then(function (result) {
          outputContent.innerHTML = escapeHtml(result.output);
          if (result.hasError) {
            outputContent.classList.add("has-error");
          }
          setStatus("");
        })
        .catch(function (err) {
          outputContent.innerHTML = escapeHtml(
            "Error: " + err.message + "\nPlease try again later."
          );
          outputContent.classList.add("has-error");
          setStatus("");
        })
        .then(function () {
          setRunning(false);
        });
    });

    if (resetBtn) {
      resetBtn.addEventListener("click", function () {
        editor.value = originalCode;
        outputWrap.style.display = "none";
        outputContent.textContent = "";
        outputContent.className = "playground-output-content";
        setStatus("");
      });
    }
  }

  function initAllPlaygrounds() {
    var playgrounds = document.querySelectorAll(".epiworld-playground");
    for (var i = 0; i < playgrounds.length; i++) {
      initPlayground(playgrounds[i]);
    }
  }

  if (typeof document$ !== "undefined" && document$.subscribe) {
    document$.subscribe(initAllPlaygrounds);
  } else {
    document.addEventListener("DOMContentLoaded", initAllPlaygrounds);
  }
})();
