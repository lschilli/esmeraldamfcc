/**
* Datei:	stack.h
* Autor:	Tobias Ramforth
* Datum:	24.01.2007
*
* Beschreibung:	Definitionen fuer Stack
* ...in Anlehnung an die Warteschlange
**/

#ifndef __RS_STACK_H_INCLUDED__
#define __RS_STACK_H_INCLUDED__

/* Eintrag im Stack ... */
typedef struct rs_stack_elem {
	struct rs_stack_elem *next;
	void *data;
	} rs_stack_elem_t;

/* Stack ... */
typedef struct {
	int n_elems;
	rs_stack_elem_t *top;
	rs_stack_elem_t *bottom;
	} rs_stack_t;

/*
 * Funktionsprototypen 
 */
rs_stack_t *rs_stack_create(void);
void rs_stack_destroy(rs_stack_t *stack);

void *rs_stack_push(rs_stack_t *stack, void *data);
void *rs_stack_pop(rs_stack_t *stack);
void *rs_stack_top(rs_stack_t *stack);
void *rs_stack_bottom(rs_stack_t *stack);

#define rs_stack_empty(s)	((s)->n_elems == 0)

#endif /* __RS_STACK_H_INCLUDED__ */
