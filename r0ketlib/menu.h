struct MENU_DEF {
    const char *text;
    void (*callback)(void);
};

struct MENU {
    const char *title;
    struct MENU_DEF entries[];
};

#define MENU_TIMEOUT  (1<<0)
#define MENU_JUSTONCE (1<<1)
#define MENU_BIG      (1<<2)
extern uint8_t menuflags;


void handleMenu(const struct MENU *the_menu);

