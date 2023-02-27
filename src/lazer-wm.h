#ifndef LAZER_WM_H_
#define LAZER_WM_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <pthread.h>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
// #include <X11/keysym.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xcb.h>

#define PADDING_SPACE 10
#define BAR_HEIGHT 25

// linked-list related structs and functions

struct node {
  void *data;
  struct node *next;
  struct node *prev;
};

struct node *create_node(void *data)
{
  struct node *tmp = malloc(sizeof(struct node));
  tmp->data = data;
  tmp->next = NULL;
  tmp->prev = NULL;
  return tmp;
};

struct node *put_at_head(struct node **head)
{
  struct node *tmp = malloc(sizeof(struct node));
  if(tmp == NULL)
    return NULL;
  if (*head == NULL)
  {
    tmp->next = NULL;
    tmp->prev = NULL;
  } else {
    tmp->prev = NULL;
    tmp->next = *head;
    tmp->next->prev = tmp;
  }

  *head = tmp;
  return tmp;
};

struct node *remove_from_head(struct node *head)
{
  if (head == NULL)
    return NULL;
  struct node *tmp = head;
  head = head->next;
  head->prev = NULL;
  free(tmp);
  tmp = NULL;
  return head;
}

void remove_node(struct node **head, struct node *node)
{
  if (*head == NULL )
    return;

  if (node->prev == NULL) {
    *head = node->next;
  } else {
    node->prev->next = node->next;
  }

  if (node->next != NULL)
    node->next->prev = node->prev;

  free(node);
}

void put_after_node(struct node *node, struct node *next)
{
  if (node == NULL || next == NULL)
    return;

  next->next = node->next;
  next->prev = node;
  node->next = next;
}

void put_before_node(struct node *node, struct node *before)
{
  before->next = node;
  before->prev = node->prev;
  node->prev = before;
}

// client struct to store data about each window

struct client {
  xcb_drawable_t id;
  uint16_t x, y, width, height;
  struct node *node;
};

struct client *find_client(xcb_drawable_t id, struct node *list)
{
  struct node *node;
  struct client *client;

  for(node = list; node != NULL; node = node->next){
    client = node->data;
    if (id == client->id) {
      return client;
    }
  };

  return NULL;
}

struct bar {
  xcb_drawable_t id;
  xcb_visualtype_t *visualtype;
  cairo_surface_t *surface;
  cairo_t *cairo;
  void (* draw) (void);
  void (* update) (void);
};

uint8_t init();
uint8_t bar_init();
uint8_t loop();
uint8_t cleanup();

void close_wm();
void spawn(char* cmd);
void kill_client(xcb_drawable_t);

void forget_window(xcb_drawable_t);

uint8_t create_bar();
void bar_draw(void);

// tools

/*

void bind_key(xcb_connection_t *connection, xcb_window_t window, uint32_t keysym, uint16_t modifiers) {
    xcb_key_symbols_t *symbols = xcb_key_symbols_alloc(connection);

    xcb_keycode_t *keycodes = xcb_key_symbols_get_keycode(symbols, keysym);
    if (!keycodes) {
        fprintf(stderr, "Error: could not get keycode for keysym 0x%lx\n", keysym);
        xcb_key_symbols_free(symbols);
        return;
    }

    xcb_keycode_t keycode = keycodes[0];
    free(keycodes);

    xcb_grab_key(connection, 1, window, modifiers, keycode, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_flush(connection);

    xcb_key_symbols_free(symbols);
}

*/

#endif // LAZER-WM_H_
