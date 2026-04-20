(function () {
  "use strict";

  var HEADER_URL =
    "https://raw.githubusercontent.com/UofUEpiBio/epiworld/refs/heads/master/epiworld.hpp";
  var CE_API_BASE = "https://godbolt.org/api";
  var CE_COMPILERS = ["g141", "g132", "g122"];
  var CE_FLAGS = "-std=c++20 -O0";

  var headerCache = null;

  function fetchHeader() {
    if (headerCache) {
      return Promise.resolve(headerCache);
    }
    return fetch(HEADER_URL)
      .then(function (response) {
        if (!response.ok) {
          throw new Error(
            "Could not download epiworld.hpp from GitHub (HTTP " +
              response.status +
              "). Check your network connection."
          );
        }
        return response.text();
      })
      .then(function (text) {
        if (text.length < 1000 || text.indexOf("EPIWORLD_HPP") === -1) {
          throw new Error(
            "Downloaded file does not look like epiworld.hpp. " +
              "GitHub may be rate-limiting requests."
          );
        }
        headerCache = text;
        return text;
      });
  }

  function tryCompile(source, header, compilerIdx) {
    if (compilerIdx >= CE_COMPILERS.length) {
      return Promise.reject(
        new Error(
          "No working compiler found on Compiler Explorer. " +
            "The service may be temporarily unavailable."
        )
      );
    }

    var compiler = CE_COMPILERS[compilerIdx];

    return fetch(CE_API_BASE + "/compiler/" + compiler + "/compile", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        Accept: "application/json",
      },
      body: JSON.stringify({
        source: source,
        options: {
          userArguments: CE_FLAGS,
          executeParameters: { args: "", stdin: "" },
          compilerOptions: { executorRequest: true },
          filters: { execute: true },
        },
        files: [{ filename: "epiworld.hpp", contents: header }],
      }),
    }).then(function (response) {
      if (response.status === 404) {
        return tryCompile(source, header, compilerIdx + 1);
      }
      if (!response.ok) {
        throw new Error(
          "Compiler Explorer returned HTTP " +
            response.status +
            " (compiler: " +
            compiler +
            ")."
        );
      }
      return response.json();
    });
  }

  function extractOutput(data) {
    var lines = [];
    var hasError = false;

    function collectLines(arr) {
      if (!Array.isArray(arr)) return "";
      return arr
        .map(function (l) {
          return l.text;
        })
        .join("\n")
        .trim();
    }

    if (data.buildResult && data.buildResult.stderr) {
      var s = collectLines(data.buildResult.stderr);
      if (s) {
        lines.push(s);
        hasError = true;
      }
    }

    if (data.stderr) {
      var s2 = collectLines(data.stderr);
      if (s2) {
        lines.push(s2);
        hasError = true;
      }
    }

    if (data.stdout) {
      lines.push(collectLines(data.stdout));
    }

    if (data.execResult) {
      var er = data.execResult;
      if (er.buildResult && er.buildResult.stderr) {
        var s3 = collectLines(er.buildResult.stderr);
        if (s3) {
          lines.push(s3);
          hasError = true;
        }
      }
      if (er.stderr) {
        var s4 = collectLines(er.stderr);
        if (s4) {
          lines.push(s4);
        }
      }
      if (er.stdout) {
        lines.push(collectLines(er.stdout));
      }
      if (typeof er.code === "number" && er.code !== 0 && !hasError) {
        lines.push("(process exited with code " + er.code + ")");
        hasError = true;
      }
    }

    var output = lines
      .filter(function (l) {
        return l;
      })
      .join("\n")
      .trim();

    if (!output) {
      output = hasError
        ? "Compilation failed with no output. The source may be too large for Compiler Explorer."
        : "(no output)";
    }

    return { output: output, hasError: hasError };
  }

  function compileAndRun(source) {
    return fetchHeader().then(function (header) {
      return tryCompile(source, header, 0).then(extractOutput);
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
          outputContent.innerHTML = escapeHtml("Error: " + err.message);
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
