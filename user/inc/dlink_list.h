/** @file dlink_list.h
 *  @brief This file defines the structure types and operations on threads.
 *
 *  @author Deepak Amin (dvamin) Faraz Shaikh (fshaikh)
 *
 *
 */


#ifndef __DLINK_LIST_H_
#define __DLINK_LIST_H_
#include<stdlib.h>
#include<stddef.h>
#include<assert.h>
#include<simics.h>

/****global typedefinitions*******/
typedef struct _DLIST_ENTRY *PDLIST_ENTRY,DLIST_ENTRY;

struct _DLIST_ENTRY {
  PDLIST_ENTRY next;
  PDLIST_ENTRY prev;
};


/**
 *  @brief  DLIST_INIT initializes the doubly linked list structure - DLIST
 *
 */


#define DLIST_INIT(pDlistEnty) do {				\
    (pDlistEnty)->next = (pDlistEnty)->prev = pDlistEnty;	\
  }while(0)

/**
 *  @brief  DLIST_EMPTY checks if the doubly linked list structure is empty
 *
 */

#define DLIST_EMPTY(pDlistEntry)			\
  ((pDlistEntry)->next == (pDlistEntry)->prev &&	\
   (pDlistEntry)->next == (pDlistEntry))

/**
 *  @brief  __dlist_insert is used to insert an element in the doubly linked list.
 *
 *
 *  @param  pDlistEntryNew - Pointer to new DLIST element.
 *  @param  pDlistEntryPrev - Pointer to current structions prev element
 *  @param  pDlistEntryNext - Pointer to current structions next element
 *  @return asserts on failure
 *
 */

static inline void __dlist_insert(
				  PDLIST_ENTRY pDlistEntryNew,
				  PDLIST_ENTRY pDlistEntryPrev,
				  PDLIST_ENTRY pDlistEntryNext)
{
  assert(DLIST_EMPTY( pDlistEntryNew ));
  pDlistEntryNew->next = pDlistEntryNext;
  pDlistEntryNew->prev = pDlistEntryPrev;
  pDlistEntryPrev->next = pDlistEntryNew;
  pDlistEntryNext->prev = pDlistEntryNew;
}


/**
 *  @brief  dlist_attach_new_head is a wrapper used to
 *          attach an element to the head of the doubly linked list.
 *          between the head and the first element in the list.
 *
 *
 *  @param  pNewHead - Pointer to new DLIST element to be placed next to head.
 *  @param  pFirstElt - Pointer to the first element in the array
 *  @return asserts on failure
 *
 */

static inline void dlist_attach_new_head(
					 PDLIST_ENTRY pNewHead,
					 PDLIST_ENTRY pFirstElt
					)
{
  __dlist_insert(pNewHead,pFirstElt->prev,pFirstElt);
}

/**
 *  @brief  dlist_push_head is a wrapper used to
 *          attach an element as the head of the doubly linked list.
 *
 *
 *  @param  pDlistEntryNew - Pointer to new DLIST element.
 *  @param  pDlistEntryHead - Pointer to head element
 *  @return asserts on failure
 *
 */

static inline void dlist_push_head(
			     PDLIST_ENTRY pDlistEntryHead,
			     PDLIST_ENTRY pDlistEntryNew)
{
  __dlist_insert( pDlistEntryNew , pDlistEntryHead , pDlistEntryHead->next );
}

/**
 *  @brief  dlist_pop_head is a wrapper used to
 *          detach an element from the head of the doubly linked list.
 *
 *
 *  @param  pDlistHead - Pointer to new DLIST element to be placed next to head.
 *  @return pDistRet - the popped element.
 *
 */

static inline PDLIST_ENTRY dlist_pop_head(PDLIST_ENTRY pDlistHead)
{
  PDLIST_ENTRY pDlistRet=NULL;

  if(DLIST_EMPTY(pDlistHead))
    return NULL;

  pDlistRet =  pDlistHead->next;

  //-- fix up the pointers --//
  pDlistRet->next->prev = pDlistRet->prev;
  pDlistRet->prev->next = pDlistRet->next;

  DLIST_INIT(pDlistRet);
  return pDlistRet;
}

/**
 *  @brief  dlist_pop_tail is a wrapper used to
 *          detach an element from the tail of the doubly linked list.
 *
 *
 *  @param  pDlistHead - Pointer to new DLIST element to be placed next to head.
 *  @return pDistRet - the popped element.
 *
 */

static inline PDLIST_ENTRY dlist_pop_tail(PDLIST_ENTRY pDlistHead)
{
  PDLIST_ENTRY pDlistRet=NULL;

  if(DLIST_EMPTY(pDlistHead))
    return NULL;

  pDlistRet =  pDlistHead->prev;

  //-- fix up the pointers --//
  pDlistRet->next->prev = pDlistRet->prev;
  pDlistRet->prev->next = pDlistRet->next;

  DLIST_INIT(pDlistRet);
  return pDlistRet;
}

/**
 *  @brief  dlist_remove_entry is a wrapper used to
 *          remove an arbitrary element from the doubly linked list.
 *
 *
 *  @param  pDlistRet - Pointer to the DLIST structure
 *  @return asserts on failure
 *
 */

static inline void dlist_remove_entry(PDLIST_ENTRY pDlistRet)
{
  assert(!DLIST_EMPTY(pDlistRet));

  //-- fix up the pointers --//
  pDlistRet->next->prev = pDlistRet->prev;
  pDlistRet->prev->next = pDlistRet->next;

  DLIST_INIT(pDlistRet);
}

/**
 *  @brief  dlist_add_tail is a wrapper used to
 *          attach an element as the tail of the doubly linked list.
 *
 *
 *  @param  pNewHead - Pointer to new DLIST element to be placed next to head.
 *  @param  pFirstElt - Pointer to the first element in the array
 *  @return asserts on failure
 *
 */

static inline void dlist_add_tail(
			     PDLIST_ENTRY pDlistEntryHead,
			     PDLIST_ENTRY pDlistEntryNew)
{
  __dlist_insert( pDlistEntryNew , pDlistEntryHead->prev , pDlistEntryHead );
}

//- traversal -//

/************Macros used for traversal across the tCB structure**************/

/*****knowing the link to any member, you can determine the pointer to tCB****/

#define DLIST_CONTAINER(ptr, type, member) \
        ( (type *) ( (char *)(ptr) - (unsigned long)(&((type *)0)->member) )  )

/*****loops for each DLIST entry in the linked tCB *******/

#define FOR_EACH_ENTRY(pDlistEntryHead,pDlistEntryTrav) \
for((pDlistEntryTrav) = (pDlistEntryHead)->next ;       \
    (pDlistEntry) != (pDlistEntryHead)          ;       \
    (pDlistEntryTrav) = (pDlistEntryTrav)->next )

/*****loops for each DLIST entry in the linked tCB (alternate) *******/

#define FOR_EACH_CONTAINER(pDlistEntryHead, ptrav, member)	\
  for(									\
      (ptrav) = DLIST_CONTAINER((pDlistEntryHead)->next,typeof(*(ptrav)),member); \
      &ptrav->member != (pDlistEntryHead);				\
      (ptrav) = DLIST_CONTAINER((ptrav)->member.next,typeof(*(ptrav)),member) \
      )


#endif // __DLINK_LIST_H_
