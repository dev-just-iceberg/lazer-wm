#include "lazer-wm.h"

static xcb_connection_t *display;
static xcb_screen_t     *screen;
static uint32_t         values[3];
static uint8_t          fullscreen = 0;
struct node *windows = NULL;
struct node *last_window = NULL;
struct client *our_window;

static struct bar _bar;

void close_wm()
{
  if (display != NULL) {
    xcb_disconnect(display);
  }
}

void kill_client(xcb_drawable_t window)
{
  if ((window == 0) || (window == screen->root))
     return;
  xcb_kill_client(display, window);
  forget_window(window);
}

void spawn(char* cmd)
{
  if (fork() == 0) {
    setsid();
    if (fork() != 0)
      _exit(0);
    execlp(cmd, cmd, (char *) NULL);
    _exit(0);
  }
  wait(NULL);
}

void move(xcb_drawable_t window, uint32_t x, uint32_t y)
{
  if (window == screen->root)
    return;

  uint32_t vals[2];

  vals[0] = x;
  vals[1] = y;
  xcb_configure_window(display, window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, vals);
  xcb_flush(display);
}

void resize(xcb_drawable_t window, uint32_t width, uint32_t height)
{
  if (window == screen->root)
    return;

  uint32_t vals[2];

  vals[0] = width;
  vals[1] = height;

  xcb_configure_window(display, window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, vals);
  xcb_flush(display);
}

int getgeom(xcb_drawable_t window, uint16_t *x, uint16_t *y, uint16_t *width, uint16_t *height)
{
  xcb_get_geometry_reply_t *geom;

  geom = xcb_get_geometry_reply(display, xcb_get_geometry(display, window), NULL);

  if (geom == NULL)
    return 0;
  *x = geom->x;
  *y = geom->y;
  *width = geom->width;
  *height = geom->height;

  free(geom);
  return 1;
}

void forget_window(xcb_drawable_t window)
{
  struct client *client;
  client = find_client(window, windows);
  if (client == NULL)
    return;
  remove_node(&windows, client->node);
  free(client);
  client = NULL;
}

void map_new_window(xcb_drawable_t new){
  struct client* client;
  struct node *node;
  uint32_t mask;
  uint32_t vals[2];


  node = put_at_head(&windows);
  if (node == NULL)
    return;

  client = malloc(sizeof(struct client));
  if(client == NULL)
    return;

  node->data = client;
  client->id = new;
  client->x = PADDING_SPACE;
  client->y = PADDING_SPACE + BAR_HEIGHT;
  client->width = screen->width_in_pixels - (PADDING_SPACE + PADDING_SPACE);
  client->height = screen->height_in_pixels - BAR_HEIGHT - (PADDING_SPACE + PADDING_SPACE);

  client->node = node;
  
  xcb_map_window(display, client->id);
  mask = XCB_CW_EVENT_MASK;
  vals[0] = XCB_EVENT_MASK_ENTER_WINDOW;
  xcb_change_window_attributes_checked(display, client->id, mask, vals);
  xcb_flush(display);

  move(client->id, client->x, client->y);
  resize(client->id, client->width, client->height);
  xcb_warp_pointer(display, client->id, XCB_NONE, 0, 0, 0, 0,
                   client->width/2 + client->x, client->height/2 + client->y);
  xcb_flush(display);

  if (last_window == NULL) {
    last_window = node;
  }
  our_window = client;
}

uint8_t loop()
{
  xcb_generic_event_t *event = xcb_wait_for_event(display);
  if (event == NULL)
    goto drawings;
  printf("event lets go %d\12", event->response_type);

  switch (event->response_type & ~0x80) {
  case XCB_MAP_REQUEST:{
    xcb_map_request_event_t *e = (xcb_map_request_event_t *) event;
    // xcb_map_window(display, e->window);
    // uint32_t vals[5];
    // vals[0] = PADDING_SPACE;
    // vals[1] = PADDING_SPACE + BAR_HEIGHT;
    // vals[2] = screen->width_in_pixels-(PADDING_SPACE + PADDING_SPACE);
    // vals[3] = screen->height_in_pixels-BAR_HEIGHT-(PADDING_SPACE + PADDING_SPACE);
    // vals[4] = 0;
    // move(e->window, vals[0], vals[1]);
    // resize(e->window, vals[2], vals[3]);
    // xcb_flush(display);
    // values[0] = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE;
    // xcb_change_window_attributes_checked(display, XCB_CW_EVENT_MASK, e->window, values);
    // xcb_set_input_focus(display, XCB_INPUT_FOCUS_POINTER_ROOT, e->window, XCB_CURRENT_TIME);
    if (e->window == _bar.id) {
      break;
    }
    map_new_window(e->window);
    break;
  }

  case XCB_DESTROY_NOTIFY:{
    xcb_destroy_notify_event_t *e = (xcb_destroy_notify_event_t *) event;
    // forget_window(e->window);
    break;
  }

  case XCB_UNMAP_NOTIFY:{
    // xcb_unmap_notify_event_t *e = (xcb_unmap_notify_event_t *) event;
    break;
  }

  case XCB_CONFIGURE_REQUEST:{
    xcb_configure_request_event_t *e = (xcb_configure_request_event_t *) event;

    move(e->window, e->x, e->y);
    resize(e->window, e->width, e->height);

    break;
  }

  case XCB_KEY_PRESS:{
    xcb_key_press_event_t *press = (xcb_key_press_event_t *) event;
    printf("key was pressed: %d\12", press->detail);
    switch (press->detail) {
    case 38:{ // q
      close_wm();
      break;
    }
    case 52:{ // w
      // kill_client(press->event);
      break;
    }
    case 41:{
      uint32_t vals[5];
      if (!fullscreen) {
        vals[0] = 0;
        vals[1] = 0;
        vals[2] = screen->width_in_pixels;
        vals[3] = screen->height_in_pixels;
        vals[4] = 0;
        fullscreen = 1;
      }else {
        vals[0] = PADDING_SPACE;
        vals[1] = PADDING_SPACE + BAR_HEIGHT;
        vals[2] = screen->width_in_pixels-(PADDING_SPACE + PADDING_SPACE);
        vals[3] = screen->height_in_pixels-BAR_HEIGHT-(PADDING_SPACE + PADDING_SPACE);
        vals[4] = 0;
        fullscreen = 0;
      }
      xcb_configure_window(display, press->child,
                           XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y
                           | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT
                           | XCB_CONFIG_WINDOW_BORDER_WIDTH, vals);
      values[0] = XCB_STACK_MODE_ABOVE;
      xcb_configure_window(display, press->child, XCB_CONFIG_WINDOW_STACK_MODE, values);
      xcb_set_input_focus(display, XCB_INPUT_FOCUS_POINTER_ROOT, press->child, XCB_CURRENT_TIME);
      xcb_flush(display);

    }
    case 45:{ // k
      struct client *client = NULL;
      uint32_t vals[] = { XCB_STACK_MODE_TOP_IF };

      if (windows == NULL)
        break;
      if(our_window->node->next == NULL){
        client = windows->data;
      } else {
        client = our_window->node->next->data;
      }
      xcb_configure_window(display, client->id, XCB_CONFIG_WINDOW_STACK_MODE, vals);
      xcb_warp_pointer(display, XCB_NONE, client->id, 0, 0, 0, 0,
                       (client->width / 2) + client->x, (client->height / 2) + client->y);
      xcb_set_input_focus(display, XCB_INPUT_FOCUS_POINTER_ROOT, client->id, XCB_CURRENT_TIME);
      our_window = client;
      xcb_flush(display);
      break;
    }
    case 46:{ // l
      // values[0] = XCB_STACK_MODE_ABOVE;
      // xcb_configure_window(display, windows[num], XCB_CONFIG_WINDOW_STACK_MODE, values);
      // xcb_set_input_focus(display, XCB_INPUT_FOCUS_POINTER_ROOT, windows[num], XCB_CURRENT_TIME);
      struct client *client = NULL;
      uint32_t vals[] = { XCB_STACK_MODE_TOP_IF };

      if (windows == NULL)
        break;
      if(our_window->node->prev == NULL){
        client = last_window->data;
      } else {
        client = our_window->node->prev->data;
      }
      xcb_configure_window(display, client->id, XCB_CONFIG_WINDOW_STACK_MODE, vals);
      xcb_warp_pointer(display, XCB_NONE, client->id, 0, 0, 0, 0,
                       (client->width / 2) + client->x, (client->height / 2) + client->y);
      xcb_set_input_focus(display, XCB_INPUT_FOCUS_POINTER_ROOT, client->id, XCB_CURRENT_TIME);
      our_window = client;
      xcb_flush(display);
      break;
    }
    case 56:{ // b
      spawn("brave-bin");
      break;
    }
    case 26:{ // e
      spawn("emacs");
      break;
    }
    case 28:{ // t
      spawn("kitty");
      break;
    }
    case 32:{ // o
      spawn("/home/mega/Games/osu/osu.AppImage");
      break;
    }
    default:
      break;
    }
  }

  case XCB_BUTTON_PRESS:{
    xcb_button_press_event_t *press = (xcb_button_press_event_t *) event;
    printf("button was pressed: %d\12", press->detail);
    break;
  }

  case XCB_ENTER_NOTIFY:{
    xcb_enter_notify_event_t *enter = (xcb_enter_notify_event_t *) event;
    if ((enter->event != 0) && (enter->event != screen->root))
      xcb_set_input_focus(display, XCB_INPUT_FOCUS_POINTER_ROOT, enter->event, XCB_CURRENT_TIME);
    break;
  }
  // case XCB_CIRCULATE_REQUEST:{
  //   xcb_circulate_request_event_t *e = (xcb_circulate_request_event_t *) event;
  //   xcb_circulate_window(display, e->window, e->place);
  //   break;
  // }
  }

  xcb_flush(display);
  free(event);

  drawings:

  _bar.draw();

  return xcb_connection_has_error(display);
}

void bar_draw(void)
{
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  char time_now[64];
  strftime(time_now, sizeof(time_now), "%Y/%m/%d - %I:%M %p", tm);
  cairo_set_source_rgb(_bar.cairo, 0.4, 0.4, 0.43137);
  cairo_paint(_bar.cairo);
  cairo_select_font_face(_bar.cairo, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(_bar.cairo, 20.0);
  cairo_set_source_rgb(_bar.cairo, 1, 1, 1);
  cairo_move_to(_bar.cairo, (screen->width_in_pixels/4)*3, 20.0);
  cairo_show_text(_bar.cairo, time_now);
  cairo_surface_flush(_bar.surface);
  xcb_flush(display);
}

uint8_t create_bar()
{
  uint32_t mask = XCB_CW_BACK_PIXEL;
  uint32_t values[1];
  values[0] = screen->white_pixel;
  _bar.id = xcb_generate_id(display);
  xcb_create_window(display,
                    XCB_COPY_FROM_PARENT,
                    _bar.id, screen->root,
                    0, 0,
                    screen->width_in_pixels, BAR_HEIGHT,
                    0,
                    XCB_WINDOW_CLASS_INPUT_OUTPUT,
                    screen->root_visual,
                    mask, values);
  xcb_map_window(display, _bar.id);

  xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);
  for(;depth_iter.rem; xcb_depth_next(&depth_iter)){
    xcb_visualtype_iterator_t visualtype_iter = xcb_depth_visuals_iterator(depth_iter.data);
    for(;visualtype_iter.rem; xcb_visualtype_next(&visualtype_iter)){
      if (screen->root_visual == visualtype_iter.data->visual_id) {
        _bar.visualtype = visualtype_iter.data;
        break;
      }
    }
  }

  _bar.surface = cairo_xcb_surface_create(display, _bar.id, _bar.visualtype,
                                               screen->width_in_pixels, BAR_HEIGHT);
  _bar.cairo = cairo_create(_bar.surface);
  _bar.draw = bar_draw;
  _bar.update = NULL;

  return xcb_connection_has_error(display);
}

uint8_t init()
{
  values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
    | XCB_EVENT_MASK_STRUCTURE_NOTIFY
    | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
    | XCB_EVENT_MASK_PROPERTY_CHANGE;
  xcb_change_window_attributes_checked(display, screen->root, XCB_CW_EVENT_MASK, values);
  xcb_ungrab_key(display, XCB_GRAB_ANY, screen->root, XCB_MOD_MASK_ANY);
  xcb_grab_button(display, 0,
                  screen->root,
                  XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
                  XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
                  screen->root, XCB_NONE, 1, XCB_MOD_MASK_4);
  xcb_grab_button(display, 0,
                  screen->root,
                  XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
                  XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
                  screen->root, XCB_NONE, 3, XCB_MOD_MASK_4);
  xcb_flush(display);
  xcb_grab_key(display, 1, screen->root, XCB_MOD_MASK_4, XCB_GRAB_ANY,
               XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
  xcb_flush(display);

  // bind_key(display, screen->root, XK_T, XCB_MOD_MASK_4);
  // bind_key(display, screen->root, XK_B, XCB_MOD_MASK_4);
  // bind_key(display, screen->root, XK_O, XCB_MOD_MASK_4);
  // bind_key(display, screen->root, XK_E, XCB_MOD_MASK_4);

  return xcb_connection_has_error(display);
}

uint8_t cleanup()
{
  xcb_flush(display);
  xcb_disconnect(display);
  return 0;
}

int main(void)
{
  uint8_t ret = 0;

  display = xcb_connect(NULL, NULL);
  ret = xcb_connection_has_error(display);
  if (ret != 0)
    return ret;
  screen = xcb_setup_roots_iterator(xcb_get_setup(display)).data;
  ret = init();
  ret = create_bar();

  while (ret == 0) {
    ret = loop();
  }

  cleanup();

  return ret;
}
