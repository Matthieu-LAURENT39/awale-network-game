#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "color.h"

// Returns the number of characters written to the output buffer
int colorize(const char *text, const char *color, const char *style, char *output)
{
    char *start = output;

    // Check input parameters and set defaults if NULL
    const char *applied_color = (color != NULL) ? color : "";
    const char *applied_style = (style != NULL) ? style : "";

    // Calculate the needed buffer size
    // size_t size = strlen(text) + strlen(applied_color) + strlen(applied_style) + strlen(COLOR_RESET) + 1;
    // char *result = malloc(size);
    // if (result == NULL)
    // {
    //     perror("Malloc failed");
    //     exit(EXIT_FAILURE);
    // }
    // Format the string
    sprintf(output, "%s%s%s%s", applied_color, applied_style, text, COLOR_RESET);
    // return result;

    return output - start;
}
