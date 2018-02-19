/**
* Datei:	stack.c
* Autor:	Tobias Ramforth
* Datum:	24.01.2007
*
* Beschreibung:	Stackverwaltung
**/

#include <string.h>

#include "memory.h"

#include "stack.h"

static rs_stack_elem_t *_stack_elem_create(void);
static void _stack_elem_destroy(rs_stack_elem_t *elem);

rs_stack_t *rs_stack_create(void)
	{
	rs_stack_t *stack;

	stack = rs_malloc(sizeof(rs_stack_t), "stack");

	stack->n_elems = 0;
  stack->top = NULL;
	stack->bottom = NULL;

	return(stack);
	}

void rs_stack_destroy(rs_stack_t *stack)
	{
	rs_stack_elem_t *elem, *next;

	if (!stack)
		return;

	elem = stack->top;
	while (elem) {
		next = elem->next;
		_stack_elem_destroy(elem);
		elem = next;
		}
	}

static rs_stack_elem_t *_stack_elem_create(void)
	{
	rs_stack_elem_t *elem;

	elem = rs_malloc(sizeof(rs_stack_elem_t), "stack element");

	return(elem);
	}

static void _stack_elem_destroy(rs_stack_elem_t *elem)
	{
	if (elem)
		rs_free(elem);
	}

void *rs_stack_push(rs_stack_t *stack, void *data)
	{
	rs_stack_elem_t *elem;

	if (!stack || !data)
		return(NULL);

	elem = _stack_elem_create();

	elem->next = NULL;
	elem->data = data;

	if (stack->top)
		elem->next = stack->top;
  else stack->bottom = elem;
  stack->top = elem;
    
	stack->n_elems++;

	return(data);		
	}

void *rs_stack_pop(rs_stack_t *stack)
	{
	void *data;
	rs_stack_elem_t *elem;

	if (!stack || !stack->top)
		return(NULL);

	elem = stack->top;
	stack->top = stack->top->next;
	if (!stack->top)
		stack->bottom = NULL;
	data = elem->data;
	_stack_elem_destroy(elem);

	stack->n_elems--;

	return(data);
	}

void *rs_stack_top(rs_stack_t *stack)
	{
	if (!stack)
		return(NULL);

	return(stack->top->data);
	}

void *rs_stack_bottom(rs_stack_t *stack)
  {
  if (!stack)
    return(NULL);
  
  return(stack->bottom->data);
  }
