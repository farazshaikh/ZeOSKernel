/** @file     variable_queue.h
 *  @brief    This file contains definitions of a 
 *            generalized queue module for data collection
 *
 *  @author   Faraz Shaikh (fshaikh) Deepak Amin (dvamin)
 *  @note     The author of this file (Faraz) is not a fan of this interface
 *            & suggests looking at the DLIST interface
 **/

#ifndef _VARIABLE_QUEUE_H
#define _VARIABLE_QUEUE_H

#include<assert.h>

/** @def Q_HEAD_EMPTY(Q_HEAD)
 *
 *  @brief Returns 1(TRUE) if there are no elements in the queue
 *  Else returns 0(FALSE)
 *  
 *  Usage: Q_NEW_HEAD(Q_HEAD_TYPE, Q_ELEM_TYPE); //create the type <br>
 *         Q_HEAD_TYPE headName; //instantiate a head of the given type
 **/

#define Q_HEAD_EMPTY(Q_HEAD)			\
  ((Q_HEAD)->next == (Q_HEAD)->prev &&		\
   (typeof(Q_HEAD))(Q_HEAD)->next == (Q_HEAD))

/** @def Q_ELEM_STAND_ALONE(Q_ELEM, LINK_NAME)
 *
 *  @brief Returns 1(TRUE) if the element is not
 *  linked against any other element in the queue
 *  Else returns 0(FALSE)
 *  
 *  Usage: Q_NEW_HEAD(Q_HEAD_TYPE, Q_ELEM_TYPE); //create the type <br>
 *         Q_HEAD_TYPE headName; //instantiate a head of the given type
 **/

#define Q_ELEM_STAND_ALONE(Q_ELEM, LINK_NAME)			\
  ((Q_ELEM)->LINK_NAME.next == (Q_ELEM)->LINK_NAME.prev &&	\
   (Q_ELEM)->LINK_NAME.next == (Q_ELEM))



/** @def Q_NEW_HEAD(Q_HEAD_TYPE, Q_ELEM_TYPE) 
 *
 *  @brief Generates a new structure of type Q_HEAD_TYPE representing the head 
 *  of a queue of elements of type Q_ELEM_TYPE. 
 *  
 *  Usage: Q_NEW_HEAD(Q_HEAD_TYPE, Q_ELEM_TYPE); //create the type <br>
 *         Q_HEAD_TYPE headName; //instantiate a head of the given type
 *
 *  @param Q_HEAD_TYPE the type you wish the newly-generated structure to have.
 *         
 *  @param Q_ELEM_TYPE the type of elements stored in the queue.
 *         Q_ELEM_TYPE must be a structure.
 *  
 **/
 
#define Q_NEW_HEAD(Q_HEAD_TYPE, Q_ELEM_TYPE)	\
  struct Q_ELEM_TYPE;				\
  typedef struct _##Q_HEAD_TYPE  {		\
    unsigned int nr_elements;			\
    struct Q_ELEM_TYPE *next;			\
    struct Q_ELEM_TYPE *prev;			\
  }Q_HEAD_TYPE

/** @def Q_NEW_LINK(Q_ELEM_TYPE)
 *
 *  @brief Instantiates a link within a structure, allowing that structure to be 
 *         collected into a queue created with Q_NEW_HEAD. 
 *
 *  Usage: <br>
 *  typedef struct Q_ELEM_TYPE {<br>
 *  Q_NEW_LINK(Q_ELEM_TYPE) LINK_NAME; //instantiate the link <br>
 *  } Q_ELEM_TYPE; <br>
 *
 *  A structure can have more than one link defined within it, as long as they
 *  have different names. This allows the structure to be placed in more than
 *  one queue simultanteously.
 *
 *  @param Q_ELEM_TYPE the type of the structure containing the link
 **/
#define Q_NEW_LINK(Q_ELEM_TYPE)				\
  struct  {						\
    struct Q_ELEM_TYPE *next;				\
    struct Q_ELEM_TYPE *prev;				\
  } 

/** @def Q_INIT_HEAD(Q_HEAD)
 *
 *  @brief Initializes the head of a queue so that the queue head can be used
 *         properly.
 *  @param Q_HEAD Pointer to queue head to initialize
 **/
#define Q_INIT_HEAD(Q_HEAD) do  {		\
    (Q_HEAD)->nr_elements = 0;			\
    (Q_HEAD)->next = (Q_HEAD)->prev =		\
      (typeof((Q_HEAD)->next))(Q_HEAD);		\
   }while(0)

/** @def Q_INIT_ELEM(Q_ELEM, LINK_NAME)
 *
 *  @brief Initializes the link named LINK_NAME in an instance of the structure  
 *         Q_ELEM. 
 *  
 *  Once initialized, the link can be used to organized elements in a queue.
 *  
 *  @param Q_ELEM Pointer to the structure instance containing the link
 *  @param LINK_NAME The name of the link to initialize
 **/
#define Q_INIT_ELEM(Q_ELEM, LINK_NAME)  do {				\
    (Q_ELEM)->LINK_NAME.next =						\
      (Q_ELEM)->LINK_NAME.prev = (Q_ELEM);				\
  }while(0)
 
/** @def Q_INSERT_FRONT(Q_HEAD, Q_ELEM, LINK_NAME)
 *
 *  @brief Inserts the queue element pointed to by Q_ELEM at the front of the 
 *         queue headed by the structure Q_HEAD. 
 *  
 *  The link identified by LINK_NAME will be used to organize the element and
 *  record its location in the queue.
 *
 *  @param Q_HEAD Pointer to the head of the queue into which Q_ELEM will be 
 *         inserted
 *  @param Q_ELEM Pointer to the element to insert into the queue
 *  @param LINK_NAME Name of the link used to organize the queue
 *
 *  @return Void (you may change this if your implementation calls for a 
 *               return value)
 **/
#define Q_INSERT_FRONT(Q_HEAD, Q_ELEM, LINK_NAME) do {	\
    typeof(Q_ELEM) pElemNext;				\
    typeof(Q_ELEM) pElemPrev;				\
							\
    pElemNext = (Q_HEAD)->next;				\
    pElemPrev = (typeof(Q_ELEM)) (Q_HEAD);		\
							\
    (Q_ELEM)->LINK_NAME.next = pElemNext;		\
    (Q_ELEM)->LINK_NAME.prev = pElemPrev;		\
							\
    /* Adjust the previous				\
       link specially handling the head	 */     	\
    if((char *)pElemPrev == (char *)(Q_HEAD)){		\
      (Q_HEAD)->next = (Q_ELEM);			\
    }else {						\
      pElemPrev->LINK_NAME.next = (Q_ELEM);		\
    }							\
    							\
    /* Adjust the next					\
       link especially handling the the head */		\
    if((char *)pElemNext == (char *) (Q_HEAD)) {	\
      (Q_HEAD)->prev = (Q_ELEM);			\
    }else { 						\
      pElemNext->LINK_NAME.prev = (Q_ELEM);		\
    } 							\
							\
    /* Increment the count of elements on head */	\
   (Q_HEAD)->nr_elements += 1;				\
  }while(0)
 

/** @def Q_INSERT_TAIL(Q_HEAD, Q_ELEM, LINK_NAME) 
 *  @brief Inserts the queue element pointed to by Q_ELEM at the end of the 
 *         queue headed by the structure pointed to by Q_HEAD. 
 *  
 *  The link identified by LINK_NAME will be used to organize the element and
 *  record its location in the queue.
 *
 *  @param Q_HEAD Pointer to the head of the queue into which Q_ELEM will be 
 *         inserted
 *  @param Q_ELEM Pointer to the element to insert into the queue
 *  @param LINK_NAME Name of the link used to organize the queue
 *
 *  @return Void (you may change this if your implementation calls for a 
 *                return value)
 **/
#define Q_INSERT_TAIL(Q_HEAD, Q_ELEM, LINK_NAME) do {	\
    typeof(Q_ELEM) pElemNext;				\
    typeof(Q_ELEM) pElemPrev;				\
    pElemNext = (typeof(Q_ELEM)) (Q_HEAD);		\
    pElemPrev = (Q_HEAD)->prev;				\
							\
    (Q_ELEM)->LINK_NAME.next = pElemNext;		\
    (Q_ELEM)->LINK_NAME.prev = pElemPrev;		\
							\
    /* Adjust the previous				\
       link specially handling the head	 */     	\
    if((char *)pElemPrev == (char *)(Q_HEAD)) {		\
      (Q_HEAD)->next = (Q_ELEM);			\
    } else {						\
      pElemPrev->LINK_NAME.next = (Q_ELEM);		\
    }							\
							\
    /* Adjust the next					\
       link especially handling the the head */		\
    if((char *)pElemNext == (char *) (Q_HEAD)) {	\
      (Q_HEAD)->prev = (Q_ELEM);			\
    } else {						\
      pElemNext->LINK_NAME.prev = (Q_ELEM);		\
    }							\
							\
    /*Increment the number of elts on the head*/	\
    (Q_HEAD)->nr_elements += 1;				\
  }while(0)


/** @def Q_GET_FRONT(Q_HEAD)
 *  
 *  @brief Returns a pointer to the first element in the queue, or NULL 
 *  (memory address 0) if the queue is empty.
 *
 *  @param Q_HEAD Pointer to the head of the queue
 *  @return Pointer to the first element in the queue, or NULL if the queue
 *          is empty
 **/

#define Q_GET_FRONT(Q_HEAD)			\
  (  (Q_HEAD_EMPTY(Q_HEAD))?NULL:(Q_HEAD)->next  )
 

/** @def Q_GET_TAIL(Q_HEAD)
 *
 *  @brief Returns a pointer to the last element in the queue, or NULL 
 *  (memory address 0) if the queue is empty.
 *
 *  @param Q_HEAD Pointer to the head of the queue
 *  @return Pointer to the last element in the queue, or NULL if the queue
 *          is empty
 **/

#define Q_GET_TAIL(Q_HEAD) \
  (  (Q_HEAD_EMPTY(Q_HEAD))?NULL:(Q_HEAD)->prev  )


/** @def Q_GET_NEXT(Q_ELEM, LINK_NAME)
 * 
 *  @brief Returns a pointer to the next element in the queue, as linked to by 
 *         the link specified with LINK_NAME. 
 *
 *  If Q_ELEM is not in a queue or is the last element in the queue, 
 *  Q_GET_NEXT should return NULL.
 *
 *  @param Q_ELEM Pointer to the queue element before the desired element
 *  @param LINK_NAME Name of the link organizing the queue
 *
 *  @return The element after Q_ELEM, or NULL if there is no next element
 **/

#define Q_GET_NEXT(Q_HEAD, Q_ELEM, LINK_NAME)				\
  ( (Q_ELEM)->LINK_NAME.next == (typeof((Q_HEAD)->next)) (Q_HEAD) ?NULL:(Q_ELEM)->LINK_NAME.next )
//  ( ((Q_ELEM)->LINK_NAME.next == (Q_ELEM)->LINK_NAME.prev)?NULL:(Q_ELEM)->LINK_NAME.next )


/** @def Q_GET_PREV(Q_ELEM, LINK_NAME)
 * 
 *  @brief Returns a pointer to the previous element in the queue, as linked to 
 *         by the link specified with LINK_NAME. 
 *
 *  If Q_ELEM is not in a queue or is the first element in the queue, 
 *  Q_GET_NEXT should return NULL.
 *
 *  @param Q_ELEM Pointer to the queue element after the desired element
 *  @param LINK_NAME Name of the link organizing the queue
 *
 *  @return The element before Q_ELEM, or NULL if there is no next element
 **/

#define Q_GET_PREV(Q_HEAD, Q_ELEM, LINK_NAME)				\
  ( (Q_ELEM)->LINK_NAME.prev == (typeof((Q_HEAD)->next)) (Q_HEAD) ?NULL:(Q_ELEM)->LINK_NAME.prev )
//  ( ((Q_ELEM)->LINK_NAME.next == (Q_ELEM)->LINK_NAME.prev)?NULL:(Q_ELEM)->LINK_NAME.prev )


/** @def Q_INSERT_AFTER(Q_HEAD, Q_INQ, Q_TOINSERT, LINK_NAME)
 *
 *  @brief Inserts the queue element Q_TOINSERT after the element Q_INQ
 *         in the queue.
 *
 *  Inserts an element into a queue after a given element. If the given
 *  element is the last element, Q_HEAD should be updated appropriately
 *  (so that Q_TOINSERT becomes the tail element)
 *
 *  @param Q_HEAD head of the queue into which Q_TOINSERT will be inserted
 *  @param Q_INQ  Element already in the queue
 *  @param Q_TOINSERT Element to insert into queue
 *  @param LINK_NAME  Name of link field used to organize the queue
 **/

#define Q_INSERT_AFTER(Q_HEAD,Q_INQ,Q_TOINSERT,LINK_NAME);


/** @def Q_INSERT_BEFORE(Q_HEAD, Q_INQ, Q_TOINSERT, LINK_NAME)
 *
 *  @brief Inserts the queue element Q_TOINSERT before the element Q_INQ
 *         in the queue.
 *
 *  Inserts an element into a queue before a given element. If the given
 *  element is the first element, Q_HEAD should be updated appropriately
 *  (so that Q_TOINSERT becomes the front element)
 *
 *  @param Q_HEAD head of the queue into which Q_TOINSERT will be inserted
 *  @param Q_INQ  Element already in the queue
 *  @param Q_TOINSERT Element to insert into queue
 *  @param LINK_NAME  Name of link field used to organize the queue
 **/

#define Q_INSERT_BEFORE(Q_HEAD,Q_INQ,Q_TOINSERT,LINK_NAME);


/** @def Q_REMOVE(Q_HEAD,Q_ELEM,LINK_NAME)
 * 
 *  @brief Detaches the element Q_ELEM from the queue organized by LINK_NAME, 
 *         and returns a pointer to the element. 
 *
 *  If Q_HEAD does not use the link named LINK_NAME to organize its elements or 
 *  if Q_ELEM is not a member of Q_HEAD's queue, the behavior of this macro
 *  is undefined.
 *
 *  @param Q_HEAD Pointer to the head of the queue containing Q_ELEM. If 
 *         Q_REMOVE removes the first, last, or only element in the queue, 
 *         Q_HEAD should be updated appropriately.
 *  @param Q_ELEM Pointer to the element to remove from the queue headed by 
 *         Q_HEAD.
 *  @param LINK_NAME The name of the link used to organize Q_HEAD's queue
 * 
 *  @return Void (if you would like to return a value, you may change this
 *                specification)
 **/

#define Q_REMOVE(Q_HEAD,Q_ELEM,LINK_NAME) do {				\
    if(Q_ELEM_STAND_ALONE(Q_ELEM,LINK_NAME)) {				\
      break;								\
    }									\
									\
    assert(!Q_HEAD_EMPTY(Q_HEAD));					\
    /* Short next end of previous */					\
    if( (char *) (Q_ELEM)->LINK_NAME.prev == (char *) (Q_HEAD) ) {	\
      (Q_HEAD)->next = (Q_HEAD)->next->LINK_NAME.next;			\
    } else {								\
      (Q_ELEM)->LINK_NAME.prev->LINK_NAME.next =			\
	(Q_ELEM)->LINK_NAME.prev->LINK_NAME.next->LINK_NAME.next;	\
    }									\
									\
    /* Short prev end of next */					\
    if( (char *) (Q_ELEM)->LINK_NAME.next ==  (char *) (Q_HEAD) ) {	\
      (Q_HEAD)->prev =  (Q_HEAD)->prev->LINK_NAME.prev;			\
    } else {								\
      (Q_ELEM)->LINK_NAME.next->LINK_NAME.prev =			\
	(Q_ELEM)->LINK_NAME.next->LINK_NAME.prev->LINK_NAME.prev;	\
    }									\
									\
    /* Book Keeping */							\
    Q_INIT_ELEM(Q_ELEM, LINK_NAME);					\
    (Q_HEAD)->nr_elements -= 1;						\
}while(0)


/** @def Q_FOREACH(CURRENT_ELEM,Q_HEAD,LINK_NAME) 
 *
 *  @brief Constructs an iterator block (like a for block) that operates
 *         on each element in Q_HEAD, in order.
 *
 *  Q_FOREACH constructs the head of a block of code that will iterate through
 *  each element in the queue headed by Q_HEAD. Each time through the loop, 
 *  the variable named by CURRENT_ELEM will be set to point to a subsequent
 *  element in the queue.
 *
 *  Usage:<br>
 *  Q_FOREACH(CURRENT_ELEM,Q_HEAD,LINK_NAME)<br>
 *  {<br>
 *  ... operate on the variable CURRENT_ELEM ... <br>
 *  }
 *
 *  If LINK_NAME is not used to organize the queue headed by Q_HEAD, then
 *  the behavior of this macro is undefined.
 *
 *  @param CURRENT_ELEM name of the variable to use for iteration. On each
 *         loop through the Q_FOREACH block, CURRENT_ELEM will point to the
 *         current element in the queue. CURRENT_ELEM should be an already-
 *         defined variable name, and its type should be a pointer to 
 *         the type of data organized by Q_HEAD
 *  @param Q_HEAD Pointer to the head of the queue to iterate through
 *  @param LINK_NAME The name of the link used to organize the queue headed
 *         by Q_HEAD.
 **/
  
#define Q_FOREACH(CURRENT_ELEM,Q_HEAD,LINK_NAME)		\
  for( (CURRENT_ELEM) = (Q_HEAD)->next ;			\
       (char *)(CURRENT_ELEM) != (char*)(Q_HEAD);		\
       (CURRENT_ELEM) = (CURRENT_ELEM)->LINK_NAME.next )

#define Q_FOREACH_DEL_SAFE(CURRENT_ELEM,Q_HEAD,LINK_NAME,SAVE_ELEM)		\
  for( (CURRENT_ELEM) = (Q_HEAD)->next , (SAVE_ELEM) = (Q_HEAD)->next->LINK_NAME.next; \
       (char *)(CURRENT_ELEM) != (char*)(Q_HEAD);		\
       (CURRENT_ELEM) = SAVE_ELEM,				\
       (SAVE_ELEM) = (SAVE_ELEM)->LINK_NAME.next )

#endif // _VARIABLE_QUEUE_H
