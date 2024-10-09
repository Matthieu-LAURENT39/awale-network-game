// colors.h
#ifndef COLORS_H
#define COLORS_H

#define COLOR_RESET "\033[0m"

#define STYLE_BOLD "\033[1m"
#define STYLE_ITALIC "\033[3m"

#define COLOR_BLACK "\033[30m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"

#define COLOR_BRIGHT_BLACK "\033[90m"
#define COLOR_BRIGHT_RED "\033[91m"
#define COLOR_BRIGHT_GREEN "\033[92m"
#define COLOR_BRIGHT_YELLOW "\033[93m"
#define COLOR_BRIGHT_BLUE "\033[94m"
#define COLOR_BRIGHT_MAGENTA "\033[95m"
#define COLOR_BRIGHT_CYAN "\033[96m"
#define COLOR_BRIGHT_WHITE "\033[97m"

// Color config
#define SERVER_SUCCESS_STYLE COLOR_GREEN
#define SERVER_INFO_STYLE COLOR_CYAN
#define SERVER_ERROR_STYLE COLOR_RED
#define SERVER_GAME_STYLE COLOR_YELLOW
#define CHAT_USERNAME_STYLE COLOR_BRIGHT_WHITE
#define CHAT_TEXT_STYLE COLOR_BRIGHT_BLACK

int colorize(const char *text, const char *color, const char *style, char *output);

#endif // COLORS_H
