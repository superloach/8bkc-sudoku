#include "8bkc-hal.h"
#include "8bkc-ugui.h"
#include "ugui.h"
#include "time.h"

#define FORE_COLOUR rgb(255, 255, 255)
#define GRID_COLOUR rgb(0,   255, 0  )
#define BACK_COLOUR rgb(0,   0,   0  )
#define ALT1_COLOUR rgb(0,   255, 255)
#define ALT2_COLOUR rgb(255, 0,   255)

#define GLOBAL_DIFF 1

/*
 Sudoku.
*/


static int grid[9][9];
static int mark[9][9][10];

int rgb(int r, int g, int b) {
	return ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (b >> 3);
}

int getkey() {
	static int old = 0xffff;
	int new = kchal_get_keys();
	int ret = (old ^ new) & new;
	old = new;
	return ret;
}

void mark_set(int x, int y, int value) {
	mark[x][y][value] = -1 * (mark[x][y][value] - 1);
}

void grid_set(int x, int y, int value) {
	grid[x][y] = value;
}

void show_number(int x, int y, int value) {
	char str[1];
	if (value == 0) {
		sprintf(str, " ");
	} else {
		sprintf(str, "%d", value);
	}
	UG_PutString(x, y, (char*)&str);
}

void mark_numbers() {
	for (int i = 0; i < 10; i++) {
		int x = 67 + ((i % 2) * 6);
		int y = 3 + ((int)(i / 2) * 8);
		show_number(x, y, i);
	}
}

void mark_cursor(int num, int colour) {
	int nx = 67 + ((num % 2) * 6);
	int ny = 3 + ((int)(num / 2) * 8);
	for (int x = nx - 1; x < nx + 5; x++) {
		UG_DrawPixel(x, ny - 1, colour);
	}
	for (int x = nx - 1; x < nx + 5; x++) {
		UG_DrawPixel(x, ny + 6, colour);
	}
	for (int y = ny - 1; y < ny + 7; y++) {
		UG_DrawPixel(nx - 1, y, colour);
	}
	for (int y = ny - 1; y < ny + 7; y++) {
		UG_DrawPixel(nx + 4, y, colour);
	}
}

void mark_display(int gx, int gy) {
	mark_numbers();
	for (int i = 0; i < 10; i++) {
		mark_cursor(i, BACK_COLOUR);
		if (mark[gx][gy][i]) mark_cursor(i, ALT2_COLOUR);
	}
}

void mark_choice(int gx, int gy) {
	int cursor = 0;
	while (1) {
		mark_display(gx, gy);
		if (mark[gx][gy][cursor]) {
			mark_cursor(cursor, ALT2_COLOUR);
			kcugui_flush();
		}
		mark_cursor(cursor, ALT1_COLOUR);

		int key = getkey();

		if (key & KC_BTN_POWER) kchal_exit_to_chooser();
		if (key & KC_BTN_LEFT) cursor = (cursor + 9) % 10;
		if (key & KC_BTN_RIGHT) cursor = (cursor + 1) % 10;
		if (key & KC_BTN_UP) cursor = (cursor + 8) % 10;
		if (key & KC_BTN_DOWN) cursor = (cursor + 2) % 10;
		if (key & KC_BTN_A) mark_set(gx, gy, cursor);
		if (key & KC_BTN_B) return;

		kcugui_flush();
	}
}

void grid_draw() {
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < KC_SCREEN_W; j++) {
			UG_DrawPixel(7 * i, j, GRID_COLOUR);
		}
	}
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < KC_SCREEN_H; j++) {
			UG_DrawPixel(j, 7 * i, GRID_COLOUR);
		}
	}
}

void grid_numbers() {
	for (int x = 0; x < 9; x++) {
		for (int y = 0; y < 9; y++) {
			show_number(7 * x + 2, 7 * y + 2, grid[x][y]);
		}
	}
}

void grid_cursor(int gx, int gy) {
	for (int x = 7 * gx; x < 7 * (gx + 1) + 1; x++) {
		UG_DrawPixel(x, 7 * gy, ALT1_COLOUR);
	}
	for (int x = 7 * gx; x < 7 * (gx + 1) + 1; x++) {
		UG_DrawPixel(x, 7 * (gy + 1), ALT1_COLOUR);
	}
	for (int y = 7 * gy; y < 7 * (gy + 1) + 1; y++) {
		UG_DrawPixel(7 * gx, y, ALT1_COLOUR);
	}
	for (int y = 7 * gy; y < 7 * (gy + 1) + 1; y++) {
		UG_DrawPixel(7 * (gx + 1), y, ALT1_COLOUR);
	}
}

void grid_choice(int gx, int gy) {
	int cursor = 0;
	while (1) {
		mark_display(gx, gy);
		if (mark[gx][gy][cursor]) {
			mark_cursor(cursor, ALT2_COLOUR);
			kcugui_flush();
		}
		mark_cursor(cursor, ALT1_COLOUR);

		int key = getkey();

		if (key & KC_BTN_POWER) kchal_exit_to_chooser();
		if (key & KC_BTN_LEFT) cursor = (cursor + 9) % 10;
		if (key & KC_BTN_RIGHT) cursor = (cursor + 1) % 10;
		if (key & KC_BTN_UP) cursor = (cursor + 8) % 10;
		if (key & KC_BTN_DOWN) cursor = (cursor + 2) % 10;
		if (key & KC_BTN_A) return grid_set(gx, gy, cursor);
		if (key & KC_BTN_B) return;

		kcugui_flush();
	}
}

int Fill(int* row, int* col) {
	for (int i = 0; i < 9; i++) {
		for (int j = 0; j < 9; j++) {
			if (grid[i][j] == 0) {
				*row = i;
				*col = j;
				return 1;
			}
		}
	}
	return 0;
}

int check_neighbors (int row, int col, int num) {
	for (int i = 0; i < 9; i++) {
		if(grid[row][i] == num) return 1;
		if(grid[i][col] == num) return 2;
	}
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			if (grid[i + row - row % 3][j + col - col % 3] == num) return 3;
		}
	}
	return 0;
}   

int fill_cells() {
	grid_numbers();
	grid_draw();
	kcugui_flush();
	for (int i = 0; i < 5000000; i++) {
		if (i == 0) i = 1;
	}
	int row = 0;
	int col = 0;
	if (Fill(&row, &col) == 0) return 1;
	for (int i = 1; i <= 9; i++) {
		if(check_neighbors(row, col, i) == 0) {
			grid[row][col] = i;
			if (fill_cells() == 1) return 1;
		}
		grid[row][col] = 0;
	}
	return 0;
}

void gen_grid() {
	srand((int)time(NULL));
	grid[rand() % 9][rand() % 9] = rand() % 9;
	for (int i = 0; i < 9;) {
		int row = rand() % 9;
		int col = rand() % 9;
		int val = rand() % 9;

		if (!check_neighbors(row, col, val)) {
			grid[row][col] = val;
			i++;
		}
	}
	fill_cells();
}

void app_main() {
	kchal_init();
	kcugui_init();
	srand((int)time(NULL));

	static int grid[9][9];
	static int full_grid[9][9];
	static int game_grid[9][9];

	// Generate grid
	gen_grid();

	// Save full_grid
	for (int i = 0; i < 9; i++) {
		for (int j = 0; j < 9; j++) {
			full_grid[i][j] = grid[i][j];
		}
	}

	// Empty random cells
	for (int i = 0; i < GLOBAL_DIFF * 12; i++) {
		grid[rand() % 9][rand() % 9] = 0;
	}

	// Save game_grid
	for (int i = 0; i < 9; i++) {
		for (int j = 0; j < 9; j++) {
			game_grid[i][j] = grid[i][j];
		}
	}

	// Empty marks
	for (int i = 0; i < 9; i++) {
		for (int j = 0; j < 9; j++) {
			for (int k = 0; k < 9; k++) {
				mark[i][j][k] = 0;
			}
		}
	}

	UG_FontSelect((UG_FONT*)&FONT_4X6);
	UG_SetForecolor(rgb(255, 255, 255));
	UG_SetBackcolor(BACK_COLOUR);
	UG_FontSetHSpace(0);
	UG_FontSetVSpace(0);

	int cx = 0;
	int cy = 0;

	while (1) {
		int key = getkey();

		UG_FillScreen(BACK_COLOUR); // clear screen

		grid_numbers();       // numbers on grid
		grid_draw();          // grid lines
		grid_cursor(cx, cy);  // main cursor
		mark_display(cx, cy); // show marks

		if (key & KC_BTN_POWER) kchal_exit_to_chooser();
		if (key & KC_BTN_LEFT)  cx = (cx + 8) % 9;
		if (key & KC_BTN_RIGHT) cx = (cx + 1) % 9;
		if (key & KC_BTN_UP)    cy = (cy + 8) % 9;
		if (key & KC_BTN_DOWN)  cy = (cy + 1) % 9;
		if (key & KC_BTN_A) grid_choice(cx, cy);
		if (key & KC_BTN_B) mark_choice(cx, cy);

		kcugui_flush();
	}
}
