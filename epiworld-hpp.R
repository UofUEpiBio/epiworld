line78 <- paste(rep("/", 78), collapse = "")

N_CALLS   <- 0L
MAX_CALLS <- 50L
INCLUDED_FILES <- character(0)

unfolder <- function(txt, rel = "include/epiworld/") {

    heads <- which(grepl("^\\s*#include \"", txt))

    if (!length(heads))
        return(txt)

    N_CALLS <<- N_CALLS + 1L

    if (N_CALLS >= MAX_CALLS) {
        warning("Max calls reached.")
        return(txt)
    }

    fns   <- gsub("^[^\"]+\"|\"$", "", txt[heads])

    head_start <- sprintf(
        "/*%s\n%1$s//\n\n Start of -%%s-\n\n%s//\n%1$s*/\n\n",
        line78,
        line78
        )

    head_end <- sprintf(
        "/*%s\n%1$s//\n\n End of -%%s-\n\n%s//\n%1$s*/\n\n",
        line78,
        line78
    )

    new_src <- txt
    for (h in rev(1L:length(fns)))
    {

        loc <- heads[h]

        fn <- trimws(paste0(rel, fns[h]))
        
        # Normalize the file path to detect true duplicates
        # This resolves relative paths like "../" to the actual file location
        fn_normalized <- normalizePath(fn, mustWork = TRUE)
        
        # Check if this file has already been included (use normalized path)
        if (fn_normalized %in% INCLUDED_FILES) {
            # Remove the include line
            new_src <- c(
                new_src[1:(loc - 1)],
                new_src[(loc + 1):length(new_src)]
            )
            next
        }
        
        # Mark this file as included (use normalized path)
        INCLUDED_FILES <<- c(INCLUDED_FILES, fn_normalized)
        
        tmp_lines <- readLines(fn, warn = FALSE)

        # Extracting relative path
        sub_rel <- gsub("[^/]+$", "", fns[h])

        # Fix the relative path construction
        if (sub_rel == "" || sub_rel == "./") {
            new_rel <- rel
        } else {
            # Remove trailing slash from rel if present, then add sub_rel
            rel_clean <- gsub("/$", "", rel)
            new_rel <- paste0(rel_clean, "/", sub_rel)
        }

        tmp_lines <- unfolder(tmp_lines, rel = new_rel)

        # Getting the filename
        new_src <- c(
          new_src[1:(loc - 1)],
          sprintf(head_start, fn),
          tmp_lines,
          sprintf(head_end, fn),
          new_src[(loc + 1):length(new_src)]
          )

        # # It should be skipping "#include"
        # if (any(grepl("^\\s*#include\\s*\"", new_src)))
        # {
        #   stop("Unexpected include found in included file: ", fn)
        # }

    }

    return(new_src)

}

rel <- "include/epiworld/"

# Reset the included files list for each run
INCLUDED_FILES <- character(0)

# Barry core
src <- readLines(paste0(rel, "epiworld.hpp"), warn = FALSE)
src_new <- unfolder(src, rel)
writeLines(src_new, "epiworld.hpp")

