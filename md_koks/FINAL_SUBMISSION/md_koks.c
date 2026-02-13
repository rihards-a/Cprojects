#include <stdio.h>
#include <stdlib.h>
#include <string.h> // used only for string copy once
#define MAX_LEN 128 // assume names are less than 128 characters - can be modified if necessary


typedef struct Node {
    char data[MAX_LEN];
    struct Node* father;
    struct Node* mother;
} Node;

// for keeping track of node name duplicates
// for having a complete linked list of all nodes from a single root
typedef struct Nodes {
    int declared; 
    int layer;
    struct Node* node;
    struct Nodes* next;
} Nodes;

// for keeping track of separate graphs
// so it's possible to update all layers when a new graph gets added
typedef struct Graph {
    int id;
    struct Graph* next;
    struct Nodes* set; // linked list of all nodes that belong
} Graph;


Node* createNode(char data[MAX_LEN]) 
{
    Node* newNode = (Node*)malloc(sizeof(Node));
    strcpy(newNode->data, data);
    newNode->father = NULL;
    newNode->mother = NULL;
    return newNode;
}

Nodes* createNodes(Node* node) 
{
    Nodes* new_node = (Nodes*)malloc(sizeof(Nodes));
    new_node->declared = 0;
    new_node->layer = 1;
    // new_node->graphID = -1;
    new_node->node = node;
    new_node->next = NULL;
    return new_node;
}

Nodes* appendNodes(Nodes* start, Nodes* node)
{
    Nodes* p = start;
    while(p->next) {
        p = p->next;
    }
    p->next = node;
}

// try to find a Nodes entry; create a new Nodes entry if it doesn't exist; 
// if declaring, then mark the entry as declared and fail when declaring it (naming it's parents) for the second time;
// return the Nodes entry it represents once found or created
Nodes* insertNodeIntoNodes(Nodes* start, Node* node, int declaring) 
{
    Nodes* crnt = start;
    Nodes* prev = NULL;
    char node_name[MAX_LEN];
    strncpy(node_name, node->data, MAX_LEN);
    node_name[MAX_LEN-1] = '\0';
    while (crnt) {
        if (strcmp(crnt->node->data, node_name) == 0) {
            if (declaring) {
                if (crnt->declared) {
                    printf("Name %s was already declared!\n", node_name);
                    exit(1);
                } else {
                    crnt->declared = 1;
                    return crnt;
                }
            } else {
                return crnt;
            }
        }
        prev = crnt;
        crnt = crnt->next;
    }
    prev->next = createNodes(node);
    prev = prev->next;
    if (declaring) {prev->declared = 1;}
    return prev;
}

Nodes* findNodeInNodes(Nodes* start, const char data[MAX_LEN]) 
{
    Nodes* tmp = start;
    while (tmp) {
        if (strcmp(tmp->node->data, data) == 0)
            return tmp;
        tmp = tmp->next;
    }
    return NULL;
}

Graph* createGraph(Nodes* start, int id) 
{
    Graph* newGraph = (Graph*)malloc(sizeof(Graph));
    newGraph->id = id;
    newGraph->set = start;
    newGraph->next = NULL;
    return newGraph;
}

void appendGraph(Graph* start, Graph* g) 
{
    Graph* p = start; 
    while (p->next) { 
        p = p->next; 
    } 
    p->next = g;
}

void addToGraph(Graph* graph, Nodes* n) 
{
    Nodes* p1 = graph->set;
    Nodes* p2 = n;
    // check for duplicates
    while (p2->next) {
        if (!findNodeInNodes(p1, p2->node->data)) {
            appendNodes(p1, p2);
        }
        p2 = p2->next;
    }
}

// crnt_g_id is used to prevent parsing the graph we are currently building when we check for already visited nodes in other graphs
int findNodeInGraphs(Graph* start, const char data[MAX_LEN], int crnt_g_id, Graph** found_in, Nodes** found) 
{
    Graph* crnt_g = start;
    while(crnt_g) {
        if (crnt_g_id > -1 && crnt_g_id == crnt_g->id) {
            crnt_g = crnt_g->next;
            continue;
        }
        Nodes* n = findNodeInNodes(crnt_g->set, data);
        if (n) {
            if (found && found_in) {
                *found_in = crnt_g;
                *found = n;
            }
            return 1;
        }
        crnt_g = crnt_g->next;
    }
    return 0;
}

void changeGraphLayers(Graph* start, int diff) 
{
    Nodes* p = start->set; 
    while (p) { 
        p->layer += diff;
        p = p->next; 
    } 
}

void unlinkGraph(Graph** start, int id) 
{
    Graph* crnt = *start;
    Graph* prev = NULL;
    while (crnt) {
        if (crnt->id == id) {
            if (prev) {
                prev->next = crnt->next;
            } else {
                *start = crnt->next;
            }
            return;
        }
        prev = crnt;
        crnt = crnt->next;
    }
}

Graph* mergeGraphs(Graph* dest, Graph* src)
{
    // check for all nodes that connect - need to be removed to prevent loops
    Nodes* tail = dest->set;
    while (tail->next) {
        tail = tail->next;
    }
    Nodes* p1 = src->set;
    while(p1) {
        if (!findNodeInNodes(dest->set, p1->node->data)) {
            Nodes* copy = malloc(sizeof(Nodes));
            *copy = *p1;
            copy->next = NULL;
            tail->next = copy;
            tail = tail->next;
        }
        p1 = p1->next;
    }
    return dest;
}

// currently visited Nodes are for loop and wrong generation detection 
// - if a node has a parent from already visited nodes that doesn't have 
// a layer assigned exactly one above, then it's either a loop, or a violation
// of the assignment requirements that generations that differ by more than 1 do not interact.
void recurseNodesAssignLayers(Nodes* currently_visited, Nodes* crnt, int layer) 
{
    crnt->layer = layer;
    Nodes* original = findNodeInNodes(currently_visited, crnt->node->data);
    if (original && original != crnt) {
        if (original->layer != layer) {
            printf("Either there is a loop, or different generations have had children together.\nIssue at node %s.\n", crnt->node->data);
            exit(1);
        }
        return;
    } 
    if (!original) {
        appendNodes(currently_visited, crnt);
    }
    // we can pass the same functionally global currently_visited set to every call, because we check layers
    if (crnt->node->mother) {
        Nodes* crnt_mother = createNodes(crnt->node->mother);
        recurseNodesAssignLayers(currently_visited, crnt_mother, layer+1);
    } 
    if (crnt->node->father) {
        Nodes* crnt_father = createNodes(crnt->node->father);
        recurseNodesAssignLayers(currently_visited, crnt_father, layer+1);
    }
}

Nodes* populate_graph(FILE* fptr) 
{
    char category[MAX_LEN]; // VARDS / TEVS / MATE
    char node[MAX_LEN];
    int f_seen = 0;
    int m_seen = 0;
    Node* crnt_node = NULL;
    Nodes* nodes_start = NULL;

    // Assume input data follows the correct format
    fscanf(fptr, "%s %s", category, node);
    if (strcmp(category, "VARDS") == 0) {
            crnt_node = createNode(node);
            nodes_start = createNodes(crnt_node);
            nodes_start->declared = 1;
            f_seen = 0;
            m_seen = 0;
    } else {
        printf("File needs to start with 'VARDS'.\n");
        exit(1);
    }
    while (fscanf(fptr, "%s %s", category, node) == 2) {
        if (strcmp(category, "VARDS") == 0) {
            crnt_node = insertNodeIntoNodes(nodes_start, createNode(node), 1)->node;
            f_seen = 0;
            m_seen = 0;
        } else if (strcmp(category, "TEVS") == 0) {
            if (f_seen) {
                printf("Cannot have 2 fathers.\n");
                exit(1);
            }
            crnt_node->father = insertNodeIntoNodes(nodes_start, createNode(node), 0)->node;
            f_seen = 1;
        } else if (strcmp(category, "MATE") == 0) {
            if (m_seen) {
                printf("Cannot have 2 mothers.\n");
                exit(1);
            }
            crnt_node->mother = insertNodeIntoNodes(nodes_start, createNode(node), 0)->node;
            m_seen = 1;
        } else {
            printf("Invalid input.\n");
            exit(1);
        }
    }
    // fclose(fptr);
    return nodes_start;
}

int main() 
{
    // ------------------------------------------------------------------
    // POPULATE THE NODE GRAPH AND KEEP A LIST OF EXISTING NODES
    // ------------------------------------------------------------------
    
    FILE* fptr = stdin;
    if(fptr == NULL) {
        printf("Not able to open the file.\n");
        return 1;
    }
    Nodes* nodes_start = populate_graph(fptr);

    // ------------------------------------------------------------------
    // WALK THE NODES LIST AND ASSIGN GENERATIONS; SPLIT GRAPHS; DETECT LOOPS
    // ------------------------------------------------------------------

    int gID = 1;
    Graph* graphs = NULL;
    Nodes* n = nodes_start;
    Nodes* next = NULL;
    while (n) {
        next = n->next; // reuse n for graphs
        n->next = NULL; // disconnect from the original list
        if (n->declared) {
            if (graphs) { 
                // check if the node has already been processed
                if (!findNodeInGraphs(graphs, n->node->data, -1, NULL, NULL)) {
                    Nodes* visited = n;
                    int layer = 1;
                    recurseNodesAssignLayers(visited, n, layer);
                    // we must check if there are currently visited nodes that were found before
                    // if so, then have to balance layers and merge the graphs
                    int at_least_one = 0;
                    Nodes* v = visited;
                    Graph* tmp_graph = createGraph(visited, gID);
                    while(v) {
                        Graph* other_graph = NULL;
                        Nodes* other_node = NULL;
                        findNodeInGraphs(graphs, v->node->data, tmp_graph->id, &other_graph, &other_node);
                        if (other_graph && other_node) { // a different graph with the same Nodes object was found -> merge together
                            at_least_one = 1;
                            int l1 = v->layer;
                            int l2 = other_node->layer;
                            if (l1 < l2) {
                                changeGraphLayers(tmp_graph, l2-l1);
                            } else if (l1 > l2) {
                                changeGraphLayers(other_graph, l1-l2);
                            }
                            // later on tmp_graph becomes a graph that's already within graphs,
                            // so we need to unlink it from the global list to prevent duplicates
                            unlinkGraph(&graphs, tmp_graph->id); 
                            tmp_graph = mergeGraphs(other_graph, tmp_graph);
                        }
                        v = v->next;
                    }
                    if (!at_least_one) { // no connections with existing graphs
                        Graph* g = tmp_graph;
                        appendGraph(graphs, g);
                        addToGraph(g, visited);
                        gID++;
                    }
                }
            } else { // first graph
                Nodes* visited = n;
                int layer = 1;
                recurseNodesAssignLayers(visited, n, layer);
                graphs = createGraph(n, gID);
                gID++;
            }
        }
        n = next;
    }

    // ------------------------------------------------------------------
    // PRINT THE GRAPHS IN DESCENDING ORDER OF LAYERS
    // ------------------------------------------------------------------

    Graph* crnt_g = graphs;
    while(crnt_g) {
        // count
        int c = 0;
        Nodes* n = crnt_g->set;
        while(n) {n = n->next; c++;}
        // create array
        Nodes** arr = malloc(c*sizeof(Nodes*));
        n = crnt_g->set;
        for (int i = 0; i < c; i++) {
            arr[i] = n;
            n = n->next;
        }
        // bubble sort
        for (int i = 0; i < c; i++) {
            for (int j = i+1; j < c; j++) {
                if (arr[i]->layer < arr[j]->layer) {
                    Nodes* tmp = arr[i];
                    arr[i] = arr[j];
                    arr[j] = tmp;
                }
            }
        }
        // print
        for (int i = 0; i < c; i++) {
            printf("%s\n", arr[i]->node->data);
        }
        printf("\n");

        free(arr);
        crnt_g = crnt_g->next;
    }

    return 0;
}
