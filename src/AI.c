#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define breadth 1		// Constants defining the four algorithms
#define depth	2
#define best	3
#define astar	4

#define MAX_LINE_LENGTH 256
#define MAX_RELATIONSHIPS 100

//Define a structure "Relationship" to represent the relationship between 2 blocks
typedef struct {
    char above;
    char below;
} Relationship;

int num_blocks;//Total number of blocks
char **goal_blocks;//Array representing the goal state
Relationship relationships[MAX_RELATIONSHIPS];//Array of relationships between blocks
int relationship_count;//Count of relationships

void sanitize_line(char *line);//Clean a line from unwanted characters
void place_block_recursively(char block, int *placed_rows, char **goal_blocks, int num_blocks, Relationship *relationships, int relationship_count);
void print_state(char **blocks,int num_blocks);//Print the current state of blocks
void move(char **blocks,int num_blocks,char block,const char *from,const char *to);//Μετακίνηση block

clock_t t1;				// Start time of the search algorithm
clock_t t2;				// End time of the search algorithm
#define TIMEOUT		60	// Maximum execution time of the program in seconds

struct tree_node{
 char **blocks;// Dynamic array of blocks since the number of blocks varies per exercise
 int h;     // Heuristic value for each node in the search tree
 int g;      // Depth for each node in the search tree from the root
 int f;     // Depending on the search algorithm, f = 0 or f = h or f = h + g
 struct tree_node *parent; // Pointer to the parent node; for the root, this is NULL
 int direction;  // Direction of the last move
 char moved_block; // The block that was moved
 char from[50];//Source location of the move
 char to[50];// Destination location of the move/
};
// The frontier represents the "Search Frontier," a set of nodes yet to be explored during the search process
// The way the frontier is managed depends on the search algorithm and is typically represented
// as a doubly linked list.
struct frontier_node{
  struct tree_node *n; // Pointer to a node in the search tree
  struct frontier_node *previous ;// Pointer to the previous frontier node
  struct frontier_node *next ;// Pointer to the next frontier node

};

struct frontier_node *frontier_head=NULL;// One end of the search frontier
struct frontier_node *frontier_tail=NULL;// The other end of the search frontier
// Function to dynamically allocate memory for a blocks array
char **allocate_blocks(int N)
 {
   char **blocks = malloc(N * sizeof(char*));
   for (int i = 0; i < N; i++)
   {
       blocks[i] = malloc(N * sizeof(char));
       memset(blocks[i],' ',N);
   }
   return blocks;
 }
// Function to free allocated memory for blocks
void free_blocks(char **blocks, int N) {
    for (int i = 0; i < N; i++) {
            free(blocks[i]);
    }
free(blocks); }

// Create a new node in the frontier
struct frontier_node *create_frontier_node(struct tree_node *node){
 struct frontier_node *new_node=malloc(sizeof(struct frontier_node));
 if(!new_node){
    perror("Memory allocation failed for frontier node");
    return NULL;
 }
 new_node->n=node;
 new_node->next=NULL;
 new_node->previous=NULL;
 return new_node;

};

// Compute heuristic value based on Manhattan Distance
int heuristic_blocks(char **current, char **goal, int num_blocks) {
    int score = 0;

    for (int i = 0; i < num_blocks; i++) {
        for (int j = 0; j < num_blocks; j++) {
            if (goal[i][j] != ' ') {
                int current_row=-1,current_col=-1;

                findBlockPosition(current,num_blocks,goal[i][j],&current_row,&current_col);

                if(current_row!=-1&&current_col!=-1){
                 score +=abs(current_row -i)+abs(current_col-j);
            }
        }
    }
 }
    return score;
}
// Function to read blocks from the ':objects' line
void read_objects(FILE *file, char **blocks, int *num_blocks) {
    char line[MAX_LINE_LENGTH];
    *num_blocks = 0;

    while (fgets(line, MAX_LINE_LENGTH, file)) {
        if (strstr(line, ":objects")) {
            printf("Found:objects line %s\n", line);

            char *token = strtok(line, " ()\n\t");
            while (token != NULL) {
                // Exclude ":objects" explicitly
                if (strcmp(token, ":objects") != 0 && strcmp(token, ")") != 0 && strcmp(token, "") != 0) {
                    blocks[*num_blocks] = strdup(token); // Store the block
                    printf("Parsed object: %s\n", blocks[*num_blocks]);
                    (*num_blocks)++;
                }
                token = strtok(NULL, " ()\n\t");
            }
            break;
        }
    }
    printf("Completed reading :objects with %d blocks.\n", *num_blocks);
}
// Function to read the initial state of blocks from the ':INIT' line
void read_init(FILE *file, char **init_state, char **blocks, int num_blocks, int *num_entries) {
    char line[MAX_LINE_LENGTH];
    char init_buffer[MAX_LINE_LENGTH * 10] = ""; // Buffer to hold the entire INIT section
    int index = 0;

    // Initialize the blocks matrix
    for (int i = 0; i < num_blocks; i++) {
        for (int j = 0; j < num_blocks; j++) {
            blocks[i][j] = ' ';
        }
    }

    // Read through the file to find the :INIT section
    while (fgets(line, MAX_LINE_LENGTH, file)) {
        sanitize_line(line); // Sanitize the line before processing

        if (strstr(line, ":INIT")) {
            printf("Found :INIT line: %s\n", line);

            // Accumulate all lines into the buffer until '))' is found
            do {
                strncat(init_buffer, line, MAX_LINE_LENGTH);
            } while (fgets(line, MAX_LINE_LENGTH, file) && !strstr(line, "))"));

            // Ensure the last line with '))' is added
            strncat(init_buffer, line, MAX_LINE_LENGTH);
            break;
        }
    }

    // Process the accumulated INIT buffer
    char *token = strtok(init_buffer, " ()\n\t");
    while (token != NULL) {
        if (strcmp(token, "CLEAR") == 0) {
            char *block = strtok(NULL, " ()\n\t");
            printf("(clear %s): Block '%s' has no other blocks on top of it.\n", block, block);
        } else if (strcmp(token, "ONTABLE") == 0) {
            char *block = strtok(NULL, " ()\n\t");
            for (int col = 0; col < num_blocks; col++) {
                if (blocks[0][col] == ' ') {
                    blocks[0][col] = block[0];
                    break;
                }
            }
            printf("(ontable %s): Block '%s' is placed on the table.\n", block, block);
        } else if (strcmp(token, "ON") == 0) {
            char *block1 = strtok(NULL, " ()\n\t");
            char *block2 = strtok(NULL, " ()\n\t");
            for (int row = 0; row < num_blocks; row++) {
                for (int col = 0; col < num_blocks; col++) {
                    if (blocks[row][col] == block2[0]) {
                        blocks[row + 1][col] = block1[0];
                        break;
                    }
                }
            }
            printf("(on %s %s): Block '%s' is placed on top of block '%s'.\n", block1, block2, block1, block2);
        } else if (strcmp(token, "HANDEMPTY") == 0) {
            printf("(handempty): The hand is empty.\n");
        } else {
            printf("Unknown token: %s\n", token);
            init_state[index] = strdup(token);
            index++;
        }
        token = strtok(NULL, " ()\n\t");
    }

    *num_entries = index;
    printf("Completed reading :INIT with %d entries.\n", *num_entries);
}


//Function to read the goal state of blocks from ':goal' line
void read_goal(FILE *file, char **goal_state, char ***goal_blocks_ptr, int *num_goals, int num_blocks) {
    char line[MAX_LINE_LENGTH];
    char goal_buffer[MAX_LINE_LENGTH * 10] = "";
    char **goal_blocks = allocate_blocks(num_blocks);
    *goal_blocks_ptr = goal_blocks;
    *num_goals = 0;


    for (int i = 0; i < num_blocks; i++) {
        for (int j = 0; j < num_blocks; j++) {
            goal_blocks[i][j] = ' ';
        }
    }

    Relationship relationships[100];
    int relationship_count = 0;

    while (fgets(line, MAX_LINE_LENGTH, file)) {
        sanitize_line(line);

        if (strstr(line, ":goal")) {
            printf("Found :goal line: %s\n", line);
            do {
                strncat(goal_buffer, line, MAX_LINE_LENGTH);
            } while (fgets(line, MAX_LINE_LENGTH, file) && !strstr(line, "))"));


            strncat(goal_buffer, line, MAX_LINE_LENGTH);
            break;
        }
    }
    printf("Goal Buffer: %s\n", goal_buffer);


    char *token = strtok(goal_buffer, " ()\n\t");
    while (token != NULL) {
        if (strcmp(token, "ON") == 0) {
            char *block1 = strtok(NULL, " ()\n\t");
            char *block2 = strtok(NULL, " ()\n\t");


            relationships[relationship_count].above = block1[0];
            relationships[relationship_count].below = block2[0];
            relationship_count++;
        }
        token = strtok(NULL, " ()\n\t");
    }


    int placed_rows[26] = {0};
    for (int i = 0; i < relationship_count; i++) {
        place_block_recursively(relationships[i].above, placed_rows,
                                 goal_blocks, num_blocks, relationships, relationship_count);
    }

    printf("Completed reading :goal.\n");


    printf("\nDebugging Goal Blocks:\n");
    for (int r = num_blocks - 1; r >= 0; r--) {
        for (int c = 0; c < num_blocks; c++) {
            printf("%c ", goal_blocks[r][c] != ' ' ? goal_blocks[r][c] : '.');
        }
        printf("\n");
    }
}
//Function that checks if a block depends on another(e.g., if it is placed on top of it)
void place_block_recursively(char block, int *placed_rows, char **goal_blocks, int num_blocks, Relationship *relationships, int relationship_count) {
    for (int r = 0; r < num_blocks; r++) {
        if (goal_blocks[r][0] == block) {
            return;
        }
    }
    int dependency_index = -1;
    //Indentify if the block has a dependecy on another block
    for (int j = 0; j < relationship_count; j++) {
        if (relationships[j].above == block) {
            dependency_index = j;
           //Recursively place the block it depends on
            place_block_recursively(relationships[j].below, placed_rows, goal_blocks, num_blocks, relationships, relationship_count);
            break;
        }
    }
    //If the block has no dependecy(i.e., it must be placed on the table)
    if (dependency_index == -1) {
        for (int col = 0; col < num_blocks; col++) {
            if (goal_blocks[0][col] == ' ') {//Find the first empty spot on the table
                goal_blocks[0][col] = block;//Place the block
                placed_rows[block - 'A'] = 0;//Record the row where it is placed
                printf("Placed block '%c' on the table at (row: 0, col: %d)\n", block, col);
                return;
            }
        }
        //If there is no space on the table,print an error message and terminate the program
        fprintf(stderr, "Error: No space on the table to place block '%c'.\n", block);
        exit(EXIT_FAILURE);
    }

    int below_row = -1;
    //Find the row where the block depends on is located
    for (int r = 0; r < num_blocks; r++) {
        if (goal_blocks[r][0] == relationships[dependency_index].below) {
            below_row = r;
            break;
        }
    }
    //If there is available space above the block it depends on
    if (below_row + 1 < num_blocks) {
        if (goal_blocks[below_row + 1][0] != ' ') {
            fprintf(stderr, "Error: Overlap detected while placing block '%c' above block '%c'.\n",
                    block, relationships[dependency_index].below);
            exit(EXIT_FAILURE);
        }
        //Place the block above the block it depends on
        goal_blocks[below_row + 1][0] = block;
        placed_rows[block - 'A'] = below_row + 1;//Record the row it was placed
        printf("Placed block '%c' above block '%c' at (row: %d, col: 0)\n",
               block, relationships[dependency_index].below, below_row + 1);
    } else {
        fprintf(stderr, "Error: Stack height exceeds maximum allowed for block '%c'.\n", block);
        exit(EXIT_FAILURE);
    }
}
//Function that checks if a string (token) is a valid predicate ("CLEAR","ONTABLE","ON","HANDEMPTY")
int is_predicate(const char *token){
  return strcmp(token,"CLEAR")==0||strcmp(token,"ONTABLE")==0||strcmp(token,"ON")==0||strcmp(token,"HAND")==0;
}
void visualize_blocks(char **blocks, int num_blocks) {
    printf("\nVisualization of Blocks:\n");

    for (int i = num_blocks - 1; i >= 0; i--) { // Start from the top row
        printf("|");
        for (int j = 0; j < num_blocks; j++) {
            if (blocks[i][j] != ' ') {
                printf("%c|", blocks[i][j]); // Print the block
            } else {
                printf(" |"); // Print empty space
            }
        }
        printf("\n");
    }

    // Print table separator
    for (int j = 0; j < num_blocks; j++) {
        printf("--");
    }
    printf("-\n");
}
//Function for cleaning a line from unwanted characters
void sanitize_line(char *line) {
    char *src = line, *dst = line;
    while (*src) {
        if (*src != '\r' && *src != '\n' && *src != '\t') {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}
//Function for visualizing the final state of the blocks
void visualize_goal_blocks(char **goal_blocks, int num_blocks) {
    printf("\nVisualization of Final Goal State:\n");

    for (int row = num_blocks - 1; row >= 0; row--) {
        printf("|");
        for (int col = 0; col < num_blocks; col++) {
            if (goal_blocks[row][col] != ' ') {
                printf("%c|", goal_blocks[row][col]);
            } else {
                printf(" |");
            }
        }
        printf("\n");
    }

    for (int col = 0; col < num_blocks; col++) {
        printf("--");
    }
    printf("-\n");
}
//Function that checks if all blocks in the goal state are present in the initial state
int validate_states(char **init_state,char **goal_state,int num_blocks){
  for(int i=0;i<num_blocks;i++){
    for(int j=0;j<num_blocks;j++){
        if (goal_state[i][j]!=' ' && init_state[i][j]!=goal_state[i][j]){
            return 0;
        }
    }
  }
  return 1;
}
//Function that checks if 2 block frids are identical
//If they are, the function return 1 ,otherwise, it returns 0.
int equal_blocks(char **blocks1,char **blocks2,int num_blocks){
  int i,j;
  for(i=0;i<num_blocks;i++){
    for(j=0;j<num_blocks;j++){
        if (blocks1[i][j]!=blocks2[i][j])
            return 0;//blocks are not equal
    }

  }
   return 1;//blocks are equal


}

//Function that check whether a newly created node in the search tree
//id identical to any of the previous nodes along the path from the root to this node.
//If there is a match with a previous node,the function returns 0;otherwise,it returns 1
int check_with_parents(struct tree_node *new_node,int num_blocks){
    struct tree_node *parent=new_node->parent;
    while(parent!=NULL)
    {
        if (equal_blocks(new_node->blocks,parent->blocks,num_blocks))
            return 0;
        parent=parent->parent;
    }
    return 1;
}

//The strcmp function is used to compare 2 strings and returns
//an integer based on the result of the comparison
//ΤThe comparison result is:
//0,if the strings are equal
//<0,if the first string (argument) is less than the second string
//>0,if the first string (argument) is greater than the second string
int get_searchAlgorithm(const char *s){
   if(strcmp(s,"breadth")==0)
        return breadth;
   if(strcmp(s,"depth")==0)
        return depth;
   if(strcmp(s,"best")==0)
        return best;
   else if(strcmp(s,"astar")==0)
        return astar;
   fprintf(stderr,"Error:Unknown algorithm '%s'\n",s);
        return -1;
}

//Function that returns the row and column of a block

void findBlockPosition(char **blocks,int num_blocks,char block,int *row,int *column){
    for(int i=0;i<num_blocks;i++){
        for(int j=0;j<num_blocks;j++){
            if(blocks[i][j]==block){
                *row=i;
                *column=j;
                return;
            }
        }
    }
    //If the block is not found ,return -1 as an indicator
    *row=-1;
    *column=-1;

}
//Function to find the Manhattan distance
int findManhattanDistance(char **blocks,int num_blocks,char block,int goal_row,int goal_col) {
   int current_row,current_col;

   findBlockPosition(blocks,num_blocks,block,&current_row,&current_col);
   if(current_row==-1 || current_col==-1)
    return 0;
   //Calculate the Manhattan distance between the current and goal positions
   return abs(current_row-goal_row)+abs(current_col-goal_col);
}
//Function to calculate the total Manhattan distance
int calculate_total_manhattan_distance(char **blocks,int num_blocks,char **goal){
   int total_distance=0;
   for(int i=0;i<num_blocks;i++){
     for(int j=0;j<num_blocks;j++){
        if(blocks[i][j]!= ' '){
            total_distance+=findManhattanDistance(blocks,num_blocks,blocks[i][j],i,j);
        }
     }
   }
   return total_distance;
}
// EXPLANATION OF THE FUNCTIONALITY OF THE FOLLOWING FUNCTIONS REGARDING THE FRONTIER:
// At the beginning of the algorithm, the frontier contains only the initial node (initial state).
// When a node expands, its children are added to the frontier.
// The algorithm selects a node from the frontier based on the search algorithm being used.
// In Breadth-First Search: The frontier is implemented as a queue (FIFO), and nodes
// are examined in the order they were added to the frontier.
// In Depth-First Search: The frontier is implemented as a stack (LIFO), and the most recently
// added node is examined first.
// In A* Search: The frontier is implemented as a priority queue, and nodes are examined based on
// their f-value, where f = g + h. Here, g represents the cost to reach the node from the initial state,
// and h represents the estimated cost from the node to the goal.
// The process continues until the goal node is found or the frontier becomes empty (indicating no solution).

int add_to_frontier(struct frontier_node **head, struct frontier_node **tail
                    , struct tree_node *node, int method) {
    if (method == breadth) return add_frontier_back(head, tail, node);
    if (method == depth) return add_frontier_front(head, tail, node);
    if (method == astar || method == best) return add_frontier_in_order(head, tail, node);
    return -1;
}

//Function called by the Depth-First Search algorithm as it adds a pointer to a new leaf node
//of the search tree at the front of the frontier
int add_frontier_front(struct frontier_node **head,struct frontier_node **tail,struct tree_node *node){
  //Crete the new frontier node
  struct frontier_node *new_node=create_frontier_node(node);
  if (!new_node) return -1;

  if(!(*head)){
    *head=*tail=new_node;
  }
  else{
    new_node->next=*head;
    (*head)->previous=new_node;
    *head=new_node;
  }
  return 0;
}

//Function called by the Breadth-First Search algorithm as it adds a pointer to a new leaf node
//of the search tree at the end of the frontier
int add_frontier_back(struct frontier_node **head,struct frontier_node **tail,struct tree_node *node){
  //Create the new frontier
  struct frontier_node *new_node=create_frontier_node(node);
  if (!new_node) return -1;

  if(!(*tail)){ //If the frontier is empty
    *head=*tail=new_node;//The new node becomes both the head and the tail
  }
  else{
    (*tail)->next=new_node;//Add the new node at the end
    new_node->previous=*tail;//Set the previous node
   *tail=new_node;//The new node becomes the new tail
  }
  return 0;
}

// Function called by the A* or Best-First Search (heuristic search algorithm) as it adds a pointer
// to a new leaf node of the search tree within the frontier.
// The frontier is always maintained in ascending order based on the f-values of the corresponding search nodes.
// The new frontier node is inserted in the correct order.
int add_frontier_in_order(struct frontier_node **head,struct frontier_node **tail,struct tree_node *node)
{
  //Create the new frontier node
  struct frontier_node *new_node = create_frontier_node(node);
    if (!new_node) return -1;

    if (!(*head)) {
        *head = *tail = new_node;
    } else {
        struct frontier_node *current = *head;//Start from the head and find the correct position based on f-value
        while (current &&
               (current->n->f < node->f ||
               (current->n->f == node->f && current->n->h < node->h))) {
            current = current->next;
        }

        if (current) {//If a suitable position is found
            new_node->next = current;//Link the new node
            new_node->previous = current->previous;
            if (current->previous) current->previous->next = new_node;
            current->previous = new_node;

            if (current == *head) *head = new_node;//If inserted at the beginning
        } else {//If inserted at the end
            (*tail)->next = new_node;
            new_node->previous = *tail;
            *tail = new_node;
        }
    }
    return 0;
}
//Function that visualizes the frontier
void print_frontier(struct frontier_node *head){
    printf("\nFrontier State:\n");
    struct frontier_node *current=head;
    while(current){
        print_state(current->n->blocks,num_blocks);
        printf("Node f: %d, g: %d, h: %d\n",current->n->f,current->n->g,current->n->h);
        current=current->next;
    }
}
//Function that checks if the current state of the blocks matches the goal state
int goal_reached(char **current_blocks,char **goal_blocks,int num_blocks){
     for(int i=0;i<num_blocks;i++){
        for(int j=0;j<num_blocks;j++){
                //Check if all blocks are in the correct positions
            if(goal_blocks[i][j]!=' '&& current_blocks[i][j]!=goal_blocks[i][j]){
                return 0;
            }
        }
     }
     return 1;

}
//Function for Depth First Search
struct tree_node* depth_first_search(struct tree_node *initial_node,char **goal_state,int num_blocks,int num_goals){
  clock_t start_time=clock();
  add_frontier_front(&frontier_head, &frontier_tail, initial_node);//Add initial node to frontier

  while(frontier_head!=NULL){
        //Check time limit
        if((double)(clock()-start_time)/CLOCKS_PER_SEC > TIMEOUT){
            printf("Time limit exceeded.\n");
            break;
        }
    struct frontier_node *current_node=frontier_head;//Process the first node
    frontier_head=frontier_head->next;//Move the frontier

    printf("Expanding node at depth %d...\n",current_node->n->g);
    printf("Current state:\n");

    for(int i=0;i<num_blocks;i++){
        printf("%s\n",current_node->n->blocks[i]);//Display the current state

    //Check if the node is the goal
    if(goal_reached(current_node->n->blocks,goal_state,num_blocks)){
        clock_t end_time=clock();
        double elapsed_time=(double)(end_time-start_time)/CLOCKS_PER_SEC;
        printf("DFS completed in %.2f seconds.\n",elapsed_time);
        return current_node->n;//If the state is goal,return the node
    //Expand node
    expand_node(current_node->n,current_node->n->blocks,num_blocks,goal_state,depth);
  }
  clock_t end_time=clock();
  double elapsed_time=(double)(end_time-start_time)/CLOCKS_PER_SEC;
  printf("DFS terminated in %.2f seconds.\n",elapsed_time);

  return NULL;
}
//Function for Breadth First Search
struct tree_node* breadth_first_search(struct tree_node *initial_node,char **goal_state,int num_blocks,int num_goals){
 clock_t start_time=clock();//Start time measurement
 add_frontier_front(&frontier_head, &frontier_tail, initial_node);

 while(frontier_head!=NULL){
     //Check time limit
    if((double)(clock()-start_time)/CLOCKS_PER_SEC > TIMEOUT){
        printf("Time limit exceeded.\n");
        break;
    }
    struct frontier_node *current_node=frontier_head;//Process 1rst node
    frontier_head=frontier_head->next;//Move the frontier

    print_state(current_node->n->blocks, num_blocks);

    //Check if the node is the goal
    if (goal_reached(current_node->n->blocks,goal_state,num_blocks)){
        clock_t end_time=clock();
        double elapsed_time=(double)(end_time-start_time)/CLOCKS_PER_SEC;
        printf("BFS completed in %.2f seconds.\n",elapsed_time);
        return current_node->n;
    }
    //Expand node
    expand_node(current_node->n,current_node->n->blocks,num_blocks,goal_state,breadth);
 }
  clock_t end_time=clock();
  double elapsed_time=(double)(end_time-start_time)/CLOCKS_PER_SEC;
  printf("BFS terminated in %.2f seconds.\n",elapsed_time);

  return NULL;
}
//Function for Astar First Search
struct tree_node* astar_search(struct tree_node *initial_node,char **goal_state,int num_blocks){
clock_t start_time=clock();//start time measurement
add_frontier_in_order(&frontier_head, &frontier_tail, initial_node);

 while(frontier_head!=NULL){
     if((double)(clock()-start_time)/CLOCKS_PER_SEC > TIMEOUT){
        printf("Time limit exceeded.\n");
        break;
    }

    struct frontier_node *current_node=frontier_head;
    frontier_head=frontier_head->next;

    if(goal_reached(current_node->n->blocks,goal_state,num_blocks)){
        clock_t end_time=clock();
        double elapsed_time=(double)(end_time-start_time)/CLOCKS_PER_SEC;
        printf("A* completed in %.2f seconds.\n",elapsed_time);
        return current_node->n;
    }
    expand_node(current_node->n,current_node->n->blocks,num_blocks,goal_state,astar);
 }
  clock_t end_time=clock();
  double elapsed_time=(double)(end_time-start_time)/CLOCKS_PER_SEC;
  printf("A* terminated in %.2f seconds.\n",elapsed_time);
 return NULL;
}
//Function for Best First Search
struct tree_node* best_first_search(struct tree_node *initial_node,char **goal_state,int num_blocks){
 clock_t start_time=clock();//start time measurement
 add_frontier_in_order(&frontier_head, &frontier_tail, initial_node);

 while(frontier_head!=NULL){
         if((double)(clock()-start_time)/CLOCKS_PER_SEC > TIMEOUT){
        printf("Time limit exceeded.\n");
        break;
    }
    struct frontier_node *current_node=frontier_head;
    frontier_head=frontier_head->next;

    if(goal_reached(current_node->n->blocks,goal_state,num_blocks)){
        clock_t end_time=clock();
        double elapsed_time=(double)(end_time-start_time)/CLOCKS_PER_SEC;
        printf("Best First Search completed in %.2f seconds.\n",elapsed_time);
        return current_node->n;
    }

    expand_node(current_node->n,current_node->n->blocks,num_blocks,goal_state,best);
 }
  clock_t end_time=clock();
  double elapsed_time=(double)(end_time-start_time)/CLOCKS_PER_SEC;
  printf("Best First Search terminated in %.2f seconds.\n",elapsed_time);
 return NULL;
}
//Heuristic function that calcultates the estimated cost to reach the goal state
int heuristic(char **blocks, char **goal, int num_blocks) {
    int h = 0;
    for (int i = 0; i < num_blocks; i++) {
        for (int j = 0; j < num_blocks; j++) {
            if (blocks[i][j] != ' ' && blocks[i][j] != goal[i][j]) {
                h++;
            }
        }
    }
    return h; // Simple estimation by counting position difference
}
//Function that records and prints the moves from the initial node to the goal node
struct tree_node* create_child(struct tree_node *parent,char **blocks,int block_index
                               ,const char *destination){
   struct tree_node *child=malloc(sizeof(struct tree_node));
   if(!child){
    perror("Memory allocation failed for child node");
    return NULL;
   }
   //Copy the parent's blocks
    child->blocks = allocate_blocks(num_blocks);
    for(int i=0;i<num_blocks;i++){
        memcpy(child->blocks[i], parent->blocks[i], num_blocks * sizeof(char));
    }

    child->moved_block=blocks[block_index/num_blocks][block_index % num_blocks];
    snprintf(child->from,50,"row: %d,col: %d",block_index/num_blocks,block_index%num_blocks);
    strncpy(child->to,destination,sizeof(child->to)-1);
    child->to[sizeof(child->to)-1]='\0';

    printf("Creating child: moved block='%c',from='%s',to='%s'\n"
           ,child->moved_block,child->from,child->to);


    move(child->blocks, num_blocks,child->moved_block,child->from,child->to);

    child->parent = parent;
    child->g = parent->g + 1;
    child->h=0;
    child->f=0;
    return child;
   };
//Function that checks if a block is clear,meaning there is no other block on top of it
int is_clear(struct tree_node *node,const char *block){
  int num_blocks=0;

  //Count the number of blocks
  for (int i=0;node->blocks[i]!=NULL;i++){
    num_blocks++;

  }
  //Find the block's position
  for(int i=0;i<num_blocks;i++){
    for(int j=0;j<num_blocks;j++){
        if(node->blocks[i][j]==block[0]){
            //If there is no block on top ,it is clear
            if(j+1<num_blocks&&node->blocks[i][j+1]!=' '){
                return 0;//Not clear
            }
            return 1;
        }
    }
  }
  //If the block was not found
  return 0;
}
//Function that expands a node by creating its child nodes
int expand_node(struct tree_node *current_node, char **blocks, int num_blocks
                ,char **goal, int method) {
    for(int row=0;row<num_blocks;row++){
            for(int col=0;col<num_blocks;col++){
                if(blocks[row][col]!=' '&& is_clear(current_node,&blocks[row][col])){
                     char block=blocks[row][col];

                     //Create a child node for moving the block to the table
                     struct tree_node *child=create_child(current_node,blocks,row*num_blocks+col,"table");
                     if(child&& check_with_parents(child,num_blocks)){
                       child->h=heuristic_blocks(child->blocks,goal,num_blocks);
                       child->f=(method==astar)?child->g+child->h:child->h;
                       add_to_frontier(&frontier_head,&frontier_tail,child,method);

                }
            }
        }
    }
    return 0;

}

//Function that prints the current state of the blocks
void print_state(char **blocks, int num_blocks) {
    for (int i = 0; i < num_blocks; i++) {
        for (int j = 0; j < num_blocks; j++) {
            printf("%c ", blocks[i][j]!=' '?blocks[i][j]:'.');
        }
        printf("\n");
    }
}

//Function to move a block from position "from" to position "to"
void move(char **blocks, int num_blocks, char block, const char *from, const char *to) {
    int from_row = -1, from_col = -1;
    int to_row = -1, to_col = -1;

    // Find the blocks's position in the current state
    for (int i = 0; i < num_blocks; i++) {
        for (int j = 0; j < num_blocks; j++) {
            if (blocks[i][j] == block) {
                from_row = i;
                from_col = j;
                break;
            }
        }
    }

    // Determine the new position "to"
    if (strcmp(to, "table") == 0) {
        for (int col = 0; col < num_blocks; col++) {
            if (blocks[0][col] == ' ') {
                to_row = 0;
                to_col = col;
                break;
            }
        }
    } else {
        for (int i = 0; i < num_blocks; i++) {
            for (int j = 0; j < num_blocks; j++) {
                if (blocks[i][j] == to[0]) {
                    to_row = i + 1;
                    to_col = j;
                    break;
                }
            }
        }
    }

    //Perform the move
    if (from_row != -1 && to_row != -1) {
        blocks[to_row][to_col] = block;
        blocks[from_row][from_col] = ' ';
        log_move(block, from, to);
    }
}

//Function that logs a block movement
void log_move(char block, const char *from, const char *to) {
    printf("Moved Block '%c' from '%s' to '%s'\n", block, from, to);
}

//Function that records and prints the moves from the initial node to the final node
//depending on the search algorithm
void print_solution_moves(struct tree_node *goal_node){
 if(!goal_node){
    printf("No solution found.\n");
    return;
 }
 char moves[100][50];
 int move_count=0;
 struct tree_node *current=goal_node;

 printf("Tracing solution path....\n");
 //Traverse from the goal node back to the initial node
 while(current->parent!=NULL){
        snprintf(moves[move_count], 50, "Move block '%c' from '%s' to '%s'",
                 current->moved_block,current->from,current->to);
        move_count++;
        current=current->parent;
 }
 printf("Solution path:\n");
 for(int i=move_count -1;i>=0;i--){
    printf("%s\n",moves[i]);
 }
}
//Function that records the moves and saves them to a file
void print_solution_moves_to_file(struct tree_node *goal_node, const char *output_file) {
    FILE *file = fopen(output_file, "w");
    if (!file) {
        perror("Error opening output file");
        return;
    }
   fprintf(file, "Solution path:\n");
   printf("File '%s' opened successfully for writing.\n", output_file);


    struct tree_node *current = goal_node;
    char moves[100][100];
    int move_count = 0;
    //Traverse from the goal node back to the initial node
    while (current&& current->parent != NULL) {
        snprintf(moves[move_count],sizeof(moves[move_count]),"Move block '%c' from '%s' to '%s'\n",
                 current->moved_block,current->from,current->to);
        printf("Recorded move: %s\n",moves[move_count]);
        move_count++;
        current = current->parent;
    }

    fprintf(file, "Solution path:\n");
    for (int i = move_count - 1; i >= 0; i--) {
        fprintf(file, "%s\n", moves[i]);
    }
    printf("Moves written to file.\n");
    fclose(file);
}

int main(int argc, char *argv[])     //argc (argument count):an integer that represents the number of parameters
                                     //given in the command line when running the program
                                    //argv (argument vector):an array of strings where each element represents
                                    //one of the arguments passed when running the program in the command line
                                    //The data entered in the command line is in the form: ./blocks_solver probBLOCKS-4-0.pddl solution.txt

{

      if (argc != 4) {
        printf("Usage: %s <algorithm> <input_file> <output_file>\n", argv[0]);
        return 1;
    }
       const char *algorithm = argv[1];
       const char *inputFile=argv[2];
       const char *outputFile=argv[3];

       printf("Algorithm: %s\n",algorithm);
       printf("Input File: %s\n",inputFile);
       printf("Output File: %s\n",outputFile);

       FILE *file=fopen(inputFile,"r"); //Open the file for reading and check if it opens successfully
         if (!file){
            perror("Error oprning file");
            return 1;
         }
         printf("File opened successfully: %s\n",inputFile);

         //Allocate memory for the data

           char **blocks=allocate_blocks(500);
           char **init_state=allocate_blocks(500);
           char **goal_state=allocate_blocks(500);
           int num_blocks=0,num_goals=0,num_entries=0;
           char **goal_blocks;

          //Read the blocks
           read_objects(file,blocks,&num_blocks);
           //Reset the file pointer to the beginning
           rewind(file);
           //Read the initial state of the blocks
           read_init(file,init_state,blocks,num_blocks,&num_entries);
           rewind(file);
           read_goal(file,goal_state,&goal_blocks,&num_goals,num_blocks);

          printf("Initial State:");
          visualize_blocks(blocks,num_blocks);

         //Create initial node
         struct tree_node *initial_node=malloc(sizeof(struct tree_node));
         initial_node->blocks = allocate_blocks(num_blocks);
         for(int i=0;i<num_blocks;i++){
            for(int j=0;j<num_blocks;j++){
                initial_node->blocks[i][j]=init_state[i][j];
            }
         }
         initial_node->g=0;
         initial_node->h=calculate_total_manhattan_distance(initial_node->blocks,num_blocks,goal_state);
         initial_node->f=initial_node->g+initial_node->h;
         initial_node->parent=NULL;
         initial_node->direction=-1;

    //Select the search algorithm
     int method = get_searchAlgorithm(algorithm);
     struct tree_node* goal_node=NULL;

      if (method == depth) {
           goal_node=depth_first_search(initial_node, goal_state, num_blocks,num_goals);
      } else if (method == breadth) {
          goal_node=breadth_first_search(initial_node, goal_state, num_blocks,num_goals);
      } else if(method==astar) {
          goal_node=astar_search(initial_node,goal_state,num_blocks);
          }else if(method==best){
          goal_node=best_first_search(initial_node,goal_state,num_blocks);
          }else{
          printf("Unknown or unimplemented algorithm: %s\n",algorithm);
          return 1;
          }

     if (goal_node) {
            printf("Solution found!\n");
            print_solution_moves(goal_node);
            print_solution_moves_to_file(goal_node,outputFile);
     } else {
           printf("No solution found.\n");
     }

    // Free allocated memory
    free_blocks(blocks,num_blocks);
    free_blocks(init_state,500);
    free_blocks(goal_state,num_goals);
    free_blocks(goal_blocks,num_blocks);

    fclose(file);

    return 0;
}
