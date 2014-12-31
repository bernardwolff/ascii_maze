/*
  From http://en.wikipedia.org/wiki/Maze_generation_algorithm#Depth-first_search
    Start at a particular cell and call it the "exit."
    Mark the current cell as visited, and get a list of its neighbors. For each neighbor, starting with a randomly selected neighbor:
        If that neighbor hasn't been visited, remove the wall between this cell and that neighbor, and then recur with that neighbor as the current cell.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>

// width and height should be odd numbers
// so that walls will surround the grid
#define WIDTH 79
#define HEIGHT 23
#define ROOM '*'
#define WALL '#'
// the exit is actually the starting position, but we're using the
// terminology from the wikipedia article
#define EXIT_X 1 // x-coordinate of the exit position
#define EXIT_Y 1 // y-coordinate of the exit position
#define NORTH 0
#define SOUTH 1
#define EAST 2
#define WEST 3
#define DEBUG 0

// this function will read a character without the need for ENTER
// copied from http://stackoverflow.com/questions/421860/capture-characters-from-standard-input-without-waiting-for-enter-to-be-pressed
// not cross-platform (doesn't work on Windows)
char getch() {
  char buf = 0;
  struct termios old = {0};
  if (tcgetattr(0, &old) < 0)
    perror("tcsetattr()");
  old.c_lflag &= ~ICANON;
  old.c_lflag &= ~ECHO;
  old.c_cc[VMIN] = 1;
  old.c_cc[VTIME] = 0;
  if (tcsetattr(0, TCSANOW, &old) < 0)
    perror("tcsetattr ICANON");
  if (read(0, &buf, 1) < 0)
    perror ("read()");
  old.c_lflag |= ICANON;
  old.c_lflag |= ECHO;
  if (tcsetattr(0, TCSADRAIN, &old) < 0)
    perror ("tcsetattr ~ICANON");
  return (buf);
}

typedef struct
{
  int x;
  int y;
} coord;

typedef struct
{
  char** cells;
  int width;
  int height;
  coord player_pos;
  int max_depth;
  // actually the goal position, but we're using the terminology from
  // the wikipedia article
  coord start_pos;
  unsigned int seed;
} maze_grid;

int debug_printf(const char *format, ...)
{
  va_list args;
  va_start(args, format);

  if (DEBUG) vprintf(format, args);

  va_end(args);
}

void initialize(maze_grid* grid)
{
  int i, j;

  grid->width = WIDTH;
  grid->height = HEIGHT;
  grid->player_pos.x = EXIT_X;
  grid->player_pos.y = EXIT_Y;
  grid->max_depth = -1;
  // using malloc here instead of static allocation to leave open
  // the possibility of the grid size being user defined at runtime
  grid->cells = (char**)malloc(sizeof(char*) * grid->width);

  for (i = 0; i < grid->width; i++)
  {
    grid->cells[i] = (char*)malloc(sizeof(char) * grid->height);
  }

  srand(grid->seed);

  debug_printf("initialized\n");
}

void clear(maze_grid* grid)
{
  int i, j;

  for (j = 0; j < grid->height; j++)
  {
    for (i = 0; i < grid->width; i++)
    {
      grid->cells[i][j] = WALL;
    }
  }

  debug_printf("cleared\n");
}

int out_of_bounds(maze_grid* grid, coord* c)
{
  return c->x < 1 || c->x >= grid->width ||
    c->y < 1 || c->y >= grid->height;
}

int is_wall(maze_grid* grid, coord* c)
{
  return grid->cells[c->x][c->y] == WALL;
}

int visited(maze_grid* grid, coord* c)
{
  return out_of_bounds(grid, c) || !is_wall(grid, c);
}

// knock down the wall that lies between the two cells
void knock_down_wall(char** cells, coord* cell1, coord* cell2)
{
  int wall_x, wall_y;
  wall_x = (cell1->x + cell2->x) / 2;
  wall_y = (cell1->y + cell2->y) / 2;

  cells[wall_x][wall_y] = ROOM;
}

void generate(maze_grid* grid, coord* cur, int depth)
{
  int r, num_neighbors, i, random_neighbor;
  coord neighbors[4], rand_neighbor;

  depth++;

  // this will make the start position (actually the goal) as deep in
  // the maze as possible
  if (depth > grid->max_depth)
  {
    grid->max_depth = depth;
    grid->start_pos = *cur;
  }

  // mark the current cell as visited
  grid->cells[cur->x][cur->y] = ROOM; 

  // rooms are 2 cells apart to account for the walls
  neighbors[NORTH] = (coord){cur->x, cur->y - 2};
  neighbors[SOUTH] = (coord){cur->x, cur->y + 2};
  neighbors[EAST] = (coord){cur->x + 2, cur->y};
  neighbors[WEST] = (coord){cur->x - 2, cur->y};

  debug_printf("generate\n");
  random_neighbor = i = rand() % 4;
  
  do
  {
    debug_printf("cur->x = %i, cur->y = %i\n", cur->x, cur->y);
    debug_printf("i = %i\n", i);
    rand_neighbor = neighbors[i];
    debug_printf("checking visited\n");
    debug_printf("rand_neighbor.x = %i, rand_neighbor.y = %i\n",
        rand_neighbor.x, rand_neighbor.y);

    if (!visited(grid, &rand_neighbor))
    {
      debug_printf("knocking down wall\n");
      knock_down_wall(grid->cells, cur, &rand_neighbor);

      generate(grid, &rand_neighbor, depth);
      debug_printf("visited\n");
    }

    i = (i + 1) % 4;
  } while (i != random_neighbor);
}

void print(maze_grid* grid)
{
  int i, j;

  debug_printf("max_depth = %i, start_pos.x = %i, start_pos.y = %i\n",
      grid->max_depth, grid->start_pos.x, grid->start_pos.y);

  for (j = 0; j < grid->height; j++)
  {
    for (i = 0; i < grid->width; i++)
    {
      if (i == grid->player_pos.x && j == grid->player_pos.y)
        printf("%c", '@');
      else if (i == grid->start_pos.x && j == grid->start_pos.y)
        printf("%c", 'X');
      else
        printf("%c", grid->cells[i][j]);
    }
    printf("\n");
  }

  printf("[%i] you are the @, goal is the X, q=quit, h=left, j=down, k=up, l=right", grid->seed);
}

void move_player(maze_grid* grid, char key)
{
  coord old_player_pos = grid->player_pos;

  switch (key)
  {
    case 'h':
      grid->player_pos.x--;
      break;
    case 'l':
      grid->player_pos.x++;
      break;
    case 'j':
      grid->player_pos.y++;
      break;
    case 'k':
      grid->player_pos.y--;
      break;
  }

  if (out_of_bounds(grid, &grid->player_pos)
      || is_wall(grid, &grid->player_pos))
    grid->player_pos = old_player_pos;
  else if (grid->player_pos.x == grid->start_pos.x &&
      grid->player_pos.y == grid->start_pos.y)
    grid->cells[old_player_pos.x][old_player_pos.y] = '!';
  else
    grid->cells[old_player_pos.x][old_player_pos.y] = ' ';
}

void game_loop(maze_grid* grid)
{
  char c;

  do
  {
    print(grid);
    fflush(stdout);
    //c = getchar();
    c = getch();
    move_player(grid, c);
    printf("\n");
  } while (c != 'q');
}

int main(int argc, char *argv[])
{
  maze_grid grid;
  time_t t;

  if (argc > 1) grid.seed = atoi(argv[1]);
  else grid.seed = (unsigned)time(&t);

  printf("using seed %i\n", grid.seed);
  debug_printf("aMAZEing\n");
  initialize(&grid);
  clear(&grid);
  generate(&grid, &(coord){EXIT_X, EXIT_Y}, 0);
  game_loop(&grid); 

  return 0;
}

