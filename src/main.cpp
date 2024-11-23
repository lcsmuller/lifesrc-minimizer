#include <fstream>
#include <iostream>

#include "commandline.h"
#include "field.h"
#include "formula.h"
#include "pattern.h"

#ifdef DEBUG_MODE
void learnCallback(void* state, int* clause) {
    (void)state;
    std::cout << "Learned clause: ";
    while (*clause != 0) { // Clause terminator
        std::cout << *clause << " ";
        ++clause;
    }
    std::cout << std::endl;
}
#endif // DEBUG_MODE

int main(int argc, char** argv) {
    Options options = {};
    if (!parseCommandLine(argc, argv, &options)) {
        return EXIT_FAILURE;
    }

    std::cout << "-- Reading pattern from file: " << options.pattern
              << std::endl;
    Pattern pat;
    std::ifstream f(options.pattern);
    if (!f) {
        std::cout << "-- Error: Cannot open " << options.pattern << std::endl;
        return 1;
    }
    try {
        pat.load(f);
    } catch (std::exception& e) {
        std::cout << "-- Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    CMergeSat* s = cmergesat_init();
#ifdef DEBUG_MODE
    cmergesat_set_learn(s, NULL, 2, learnCallback);
#endif // DEBUG_MODE

    std::cout << "-- Building formula for " << options.evolutions
              << " evolution steps..." << std::endl;
    Field** fields = (Field**)malloc(sizeof(Field*) * (options.evolutions + 1));
    if (!fields) {
        std::cerr << "-- Error: Memory allocation for fields failed."
                  << std::endl;
        cmergesat_release(s);
        return EXIT_FAILURE;
    }

    for (int g = 0; g <= options.evolutions; ++g) {
        int width = pat.width();
        int height = pat.height();

        // Adjust field size based on evolution and growth options
        if (options.grow) {
            int growth = (options.backwards) ? (options.evolutions - g) : g;
            width += 2 * growth;
            height += 2 * growth;
        }

        // Create field for the current generation
        fields[g] = Field_create(s, width, height);
        if (!fields[g]) {
            std::cerr << "-- Error: Field creation failed for generation " << g
                      << "." << std::endl;
            for (int i = 0; i < g; ++i) {
                Field_destroy(fields[i]);
            }
            free(fields);
            cmergesat_release(s);
            return EXIT_FAILURE;
        }

        // Add transitions between generations
        if (g > 0) {
            transition(s, fields[g - 1], fields[g]);
        }
    }

    if (options.backwards) {
        std::cout << "-- Setting pattern constraint on last generation..."
                  << std::endl;
        patternConstraint(s, fields[options.evolutions], pat);
    } else {
        std::cout << "-- Setting pattern constraint on first generation..."
                  << std::endl;
        patternConstraint(s, fields[0], pat);
    }

    std::cout << "-- Solving formula..." << std::endl;
    if (!cmergesat_solve(s)) {
        std::cerr
            << "-- Formula is not solvable. The selected pattern is probably "
               "too restrictive!"
            << std::endl;
        for (int g = 0; g <= options.evolutions; ++g) {
            Field_destroy(fields[g]);
        }
        free(fields);
        cmergesat_release(s);
        return EXIT_FAILURE;
    }

    std::cout << std::endl;
    for (int g = 0; g <= options.evolutions; ++g) {
        if (options.backwards) {
            if (g == 0) {
                std::cout << "-- Initial generation:" << std::endl;
            } else if (g == options.evolutions) {
                std::cout << "-- Evolves to final generation (from pattern):"
                          << std::endl;
            } else {
                std::cout << "-- Evolves to:" << std::endl;
            }
        } else {
            if (g == 0) {
                std::cout << "-- Initial generation (from pattern):"
                          << std::endl;
            } else if (g == options.evolutions) {
                std::cout << "-- Evolves to final generation:" << std::endl;
            } else {
                std::cout << "-- Evolves to:" << std::endl;
            }
        }
        Field_print(s, fields[g], stdout);
        std::cout << std::endl;
    }

    for (int g = 0; g <= options.evolutions; ++g) {
        Field_destroy(fields[g]);
    }
    free(fields);
    cmergesat_release(s);

    return EXIT_SUCCESS;
}
