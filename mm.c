/* 

 * this is a 32 bit implementation.	
 * This allocator uses the size of a pointer, e.g., sizeof(void *), to
 * define the size of a word.  This allocator also uses the standard
 * type uintptr_t to define unsigned integers that are the same size
 * as a pointer, i.e., sizeof(uintptr_t) == sizeof(void *).


	free block status  === | HDR | FREE BLKP | FREE BLKP | unwanted data | FTR 
	Data block status  === | HDR | DATA | FTR

	We implemented segregated free list, and a sort of first fit implementation.

	Data stuctures used are Hash Map , Linked List ,and idea of stack.
	
	the list starting adresses are between the header and footer of prolouge.
	
	we made 100 parallel active linked lists starting from heap_listp + DSIZE to heap_listp + 102*WSIZE



	HEAP ==== padding | Prologue HDR | HASH MAP(100 indexes) | Prologue Footer | Allocated and free BLOCKS | Epilogue 

					
	Each free block is placed in a particular index of the hash map according to the size of the block 
	First index has blocks of range 0 to 25 . Second index has from 26 to 50 and so on. 
   
	In mm_realloc we checked for any free blocks adjacent to the bp we wanted to reallocate instead of calling the malloc everytime
	
	we created two new functions push and pop which adds and removes the block from the linked list.	 	
	 
 */

/********************************************************************************************************
 *			 © Achut .Nandam 
 *			   Bharath .Mutyala
 *				Dhirubhai Ambani Institute for Information and Communication Technology 
 *					India
 ********************************************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "memlib.h"
#include "mm.h"

team_t team = {
  /* Team name */
  "Infinite_Loop",
  /* First member's full name */
  "Achut Nandam",
  /* First member's email address */
  "201201169@daiict.ac.in",
  /* Second member's full name (leave blank if none) */
  "Bharath Mutyala",
  /* Second member's email address (leave blank if none) */
  "201201141@daiict.ac.in"
};

/* Basic constants and macros: */
#define WSIZE      sizeof(void *)	/* Word and header/footer size (bytes) */
#define DSIZE      (2 * WSIZE)	/* Doubleword size (bytes) */
#define CHUNKSIZE  (1 << 12)	/* Extend heap by this amount (bytes) */

#define MAX(x, y)  ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word. */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p. */
#define GET(p)       (*(uintptr_t *)(p))
#define PUT(p, val)  (*(uintptr_t *)(p) = (val))

/* Read the size and allocated fields from address p. */
#define GET_SIZE(p)   (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)  (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer. */
#define HDRP(bp)  ((char *)(bp) - WSIZE)
#define FTRP(bp)  ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks. */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Global variables: */
static char *heap_listp;	/* Pointer to first block */
static int index_start;		/*points the start of the first free block in the list */
static int num_free;	/*no: of free blocks */

/* Function prototypes for internal helper routines: */
static void *coalesce (void *bp);
static void *extend_heap (size_t words);
static void *find_fit (size_t asize);
static void place (void *bp, size_t asize);

/*self created list addition and deletion*/
static void push (void *bp);
static void pop (void *bp);

/* Function prototypes for heap consistency checker routines: */
static void checkblock (void *bp);
static void checkheap (bool verbose);
static void printblock (void *bp);

/* 
 * Requires:
 *   None.
 *
 * Effects:
 *   Initialize the memory manager.  Returns 0 if the memory manager was
 *   successfully initialized and -1 otherwise.
 */
int
mm_init (void)
{

  /* Create the initial empty heap. */
  if ((heap_listp = mem_sbrk (104 * WSIZE)) == (void *) -1)
    return (-1);
  PUT (heap_listp, 0);		/* Alignment padding */
  PUT (heap_listp + (1 * WSIZE), PACK (51 * DSIZE, 1));	/* Prologue header */
  /*creates an array of pointer for the base adress of free list  */
  int i = 2;
  for (; i < 102; i++)
    {
      PUT (heap_listp + (i * WSIZE), 0);
    }
  PUT (heap_listp + (102 * WSIZE), PACK (51 * DSIZE, 1));	/* Prologue footer */
  PUT (heap_listp + (103 * WSIZE), PACK (0, 1));	/* Epilogue header */
  heap_listp += (DSIZE);

  index_start = 300;
  num_free = 0;
  /* Extend the empty heap with a free block of CHUNKSIZE bytes. */
  if (extend_heap (CHUNKSIZE / WSIZE) == NULL)
    return (-1);
  return (0);
}

/********** SAME AS GIVEN *********/
/* 
 * Requires:
 *   None.
 *
 * Effects:
 *   Allocate a block with at least "size" bytes of payload, unless "size" is
 *   zero.  Returns the address of this block if the allocation was successful
 *   and NULL otherwise.
 */
void *
mm_malloc (size_t size)
{
  size_t asize;			/* Adjusted block size */
  size_t extendsize;		/* Amount to extend heap if no fit */
  void *bp;

  /* Ignore spurious requests. */
  if (size == 0)
    return (NULL);

  /* Adjust block size to include overhead and alignment reqs. */
  if (size <= DSIZE)
    asize = 2 * DSIZE;
  else
    asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);

  /* Search the free list for a fit. */
  if ((bp = find_fit (asize)) != NULL)
    {
      place (bp, (uintptr_t) asize);
      return (bp);
    }

  /* No fit found.  Get more memory and place the block. */
  extendsize = MAX (asize, CHUNKSIZE);
  if ((bp = extend_heap (extendsize / WSIZE)) == NULL)
    return (NULL);
  place (bp, (uintptr_t) asize);
  return (bp);
}

/************SAME AS GIVEN********/
/* 
 * Requires:
 *   "bp" is either the address of an allocated block or NULL.
 *
 * Effects:
 *   Free a block.
 */

void
mm_free (void *bp)
{
  size_t size;

  /* Ignore spurious requests. */
  if (bp == NULL)
    return;

  /* Free and coalesce the block. */
  size = GET_SIZE (HDRP (bp));
  PUT (HDRP (bp), PACK (size, 0));
  PUT (FTRP (bp), PACK (size, 0));

  coalesce (bp);
}

/*
 * Requires:
 *   "ptr" is either the address of an allocated block or NULL.
 *
 * Effects:
 *   Reallocates the block "ptr" to a block with at least "size" bytes of
 *   payload, unless "size" is zero.  If "size" is zero, frees the block
 *   "ptr" and returns NULL.  If the block "ptr" is already a block with at
 *   least "size" bytes of payload, then "ptr" may optionally be returned.
 *   Otherwise, a new block is allocated and the contents of the old block
 *   "ptr" are copied to that new block.  Returns the address of this new
 *   block if the allocation was successful and NULL otherwise.
 */

void *
mm_realloc (void *ptr, size_t size)
{

  /* If size == 0 then this is just free, and we return NULL. */
  if (size == 0)
    {
      mm_free (ptr);
      return NULL;
    }

  /* If oldptr is NULL, then this is just malloc. */
  if (ptr == NULL)
    return mm_malloc (size);


  bool inc = 0;
  void *oldptr = ptr;
  size_t tempsize;
  size_t excsize;
  void *newptr;
  void *temp;


  //get allocation status of next and previous free blocks.  
  size_t prev_alloc = GET_ALLOC (FTRP (PREV_BLKP (oldptr)));
  size_t next_alloc = GET_ALLOC (HDRP (NEXT_BLKP (oldptr)));

  //get size of the current block
  size_t oldsize = GET_SIZE (HDRP (oldptr));


  //if new size is greater than the oldsize .
  if (oldsize < size + DSIZE)
    inc = 1;

// if the size's not incremented then this is active 
  if (!inc && (oldsize - size - DSIZE) > (2 * DSIZE))
    {


      if (size <= DSIZE)
	size = 2 * DSIZE;
      else
	size = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);	// align size

	// if the diff in size is good enough to divide the block into two.
      if ((oldsize - size) > (2 * DSIZE))
	{

		// set the new realloced block's header and footer.
	  PUT (HDRP (oldptr), PACK (size, 1));
	  PUT (FTRP (oldptr), PACK (size, 1));

	  // set new ptr to old ptr
	  newptr = oldptr;

	  // reset old pointer to the new (empty) block
	  oldptr = (NEXT_BLKP (newptr));

	  //set header and footer for new (empty) block with remaining size
	  PUT (HDRP (oldptr), PACK (oldsize - size, 0));
	  PUT (FTRP (oldptr), PACK (oldsize - size, 0));

	  //coalesce the newly created block
	  coalesce (oldptr);

	  //return new pointer which is the same 
	  return newptr;
	}
    }

// if the diff in size is not good enough to divide the block into two.
  else if (!inc)
    {
      return ptr;
    }

  // if the requested size is more than the present size.   
  else
    {
	// if both the adjacent blocks are free and the expanded size is greater than the requested size.
      if (!next_alloc && !prev_alloc
	  && ((GET_SIZE (HDRP (PREV_BLKP (oldptr)))) +
	      (GET_SIZE (HDRP (NEXT_BLKP (oldptr)))) + oldsize) >=
	  (size + DSIZE))
	{

	//remove the free blocks from the appropriate list .
	  pop (PREV_BLKP (oldptr));
	  pop (NEXT_BLKP (oldptr));
	
	  //set new ptr to the prev block.  
	  newptr = PREV_BLKP (oldptr);

	  //setting temp to next block.
	  temp = NEXT_BLKP (oldptr);

	  //temp size is size of next + prev free blocks.
	  tempsize = GET_SIZE (FTRP (newptr)) + GET_SIZE (FTRP (temp));

	  /* Adjust block size to include overhead and alignment reqs. */
	  if (size <= DSIZE)
	    size = 2 * DSIZE;
	  else
	    size = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

	// checking if the size available - requested size is good that we can create a new free block. 
	  if ((tempsize + oldsize) < (size + 2 * DSIZE))
	    size = tempsize + oldsize;

	  //set header to reflect new size.
	  PUT (HDRP (newptr), PACK (size, 1));

	  // copy size and copy memory from old block to new block
	  excsize = GET_SIZE (HDRP (oldptr));
	  memcpy (newptr, oldptr, excsize);

	  //set footer to reflect new (expanded) size. 
	  PUT (FTRP (newptr), PACK (size, 1));

	  // checking if the size available - requested size is good that we can create a new free block. 
	  if ((tempsize + oldsize) >= (size + 2 * DSIZE))
	    {

	      // setting the new next pointer for making it a free block.
	      temp = NEXT_BLKP (newptr);

	      // set header and footer for new block with remaining size
	      PUT (HDRP (temp), PACK (tempsize + oldsize - size, 0));
	      PUT (FTRP (temp), PACK (tempsize + oldsize - size, 0));

	      //add new block to the free list
	      push (temp);
	    }

	  //return new block.
	  return newptr;
	}

     // if the next block is free and the expanded size is greater than the requested size.
      else if (!next_alloc
	       && ((GET_SIZE (HDRP (NEXT_BLKP (oldptr)))) + oldsize) >=
	       (size + DSIZE))
	{
 	//remove next block from the free list. 
	  pop (NEXT_BLKP (ptr));

	  //temp set to next block
	  temp = NEXT_BLKP (oldptr);

	  //temp size is size of next block
	  tempsize = GET_SIZE (FTRP (temp));

	  // Adjust block size to include overhead and alignment reqs.
	  if (size <= DSIZE)
	    size = 2 * DSIZE;
	  else
	    size = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

	 // checking if the size available - requested size is good that we can create a new free block.
	  if ((tempsize + oldsize) < (size + 2 * DSIZE))
	    size = tempsize + oldsize;

	  //set header and footer for newly reallocated block
	  PUT (HDRP (oldptr), PACK (size, 1));
	  PUT (FTRP (oldptr), PACK (size, 1));

	 // checking if the size available - requested size is good that we can create a new free block.
	  if ((tempsize + oldsize) >= (size + 2 * DSIZE))
	    {

	      // set new pointer to the new block
	      newptr = NEXT_BLKP (oldptr);

	      // set header and foot for new block with remaining size
	      PUT (HDRP (newptr), PACK (tempsize + oldsize - size, 0));
	      PUT (FTRP (newptr), PACK (tempsize + oldsize - size, 0));

	      //add new block to the free list
	      push (newptr);
	    }
	  //return realloced block pointer.
	  return oldptr;
	}

    // if the prev block is free and the expanded size is greater than the requested size.
      else if (!prev_alloc
	       && ((GET_SIZE (HDRP (PREV_BLKP (oldptr)))) + oldsize) >=
	       (size + DSIZE))
	{

	  //set new ptr to the prev block since this will be the address of the expanded block.  
	  newptr = PREV_BLKP (oldptr);

	  //tempsize is size of prev block. 
	  tempsize = GET_SIZE (FTRP (newptr));

	  //remove prev block from the free list
	  pop (PREV_BLKP (oldptr));

	  // Adjust block size to include overhead and alignment reqs.
	  if (size <= DSIZE)
	    size = 2 * DSIZE;
	  else
	    size = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

// checking if the size available - requested size is good that we can create a new free block.
	  if ((tempsize + oldsize) < (size + 2 * DSIZE))
	    size = tempsize + oldsize;

	  //set header to reflect new size.
	  PUT (HDRP (newptr), PACK (size, 1));

	  // calculate copy size and copy memory from old block to new block
	  excsize = GET_SIZE (HDRP (oldptr));
	//retaining the old data into the 	 
	 memcpy (newptr, oldptr, excsize);

	  //set footer to reflect new (expanded) size.
	  PUT (FTRP (newptr), PACK (size, 1));	// resize old 

	  // checking if the size available - requested size is good that we can create a new free block.
	  if ((tempsize + oldsize) >= (size + 2 * DSIZE))
	    {

	      // set new pointer to the new block
	      temp = NEXT_BLKP (newptr);

	      // set header and footer for new  block with remaining size
	      PUT (HDRP (temp), PACK (tempsize + oldsize - size, 0));
	      PUT (FTRP (temp), PACK (tempsize + oldsize - size, 0));

	      //add new block to the free list
	      push (temp);
	    }
	  //return newly realloced block.
	  return newptr;
	}

      //else if next and previous blocks are allocated.
 
	//from here same as given before.

      //set new pointer to newly allocated block of size
      newptr = mm_malloc (size);

      //calculate copy size with a maximum of size.  (size should never be less than excsize but left in for safety)
      excsize = GET_SIZE (HDRP (oldptr));
      if (size < excsize)
	excsize = size;

      //copy memory to new block
      memcpy (newptr, oldptr, excsize);

      //free old memory block
      mm_free (oldptr);

      //return new memory block.
      return newptr;
    }
	// it asked me to return something anyways this is never occuring .
	return 0;
}

/*
 * The following routines are internal helper routines.
 */

/*
 * Requires:
 *   "bp" is the address of a newly freed block.
 *
 * Effects:
 *   Perform boundary tag coalescing.  Returns the address of the coalesced
 *   block.
 */
static void *
coalesce (void *bp)
{
  //get allocation status of next and previous free blocks.  
  size_t prev_alloc = GET_ALLOC (FTRP (PREV_BLKP (bp)));
  size_t next_alloc = GET_ALLOC (HDRP (NEXT_BLKP (bp)));

  //get size of this block.  
  size_t size = GET_SIZE (HDRP (bp));

  //if next and previous blocks are allocated 
  if (prev_alloc && next_alloc)
    {				/* Case 1 */

      //add this block to the free list and return.
      push (bp);
      return bp;
    }

  //if next block is free .
  else if (prev_alloc && !next_alloc)
    {				/* Case 2 */

      //remove next block from the free list .
      pop (NEXT_BLKP (bp));

      // add the next block size.  
      size += GET_SIZE (HDRP (NEXT_BLKP (bp)));

      //set header and footer  new free block . 
      PUT (HDRP (bp), PACK (size, 0));
      PUT (FTRP (bp), PACK (size, 0));	// change to   PUT (FTRP (NEXT_BLKP(bp)), PACK (size, 0));

      push (bp);
    }

  //if previous block is free.  
  else if (!prev_alloc && next_alloc)
    {				/* Case 3 */

      //remove previous block from the free list .
      pop (PREV_BLKP (bp));

      // add the previous block size. 
      size += GET_SIZE (HDRP (PREV_BLKP (bp)));

      //set header and footer for the new combined block with new combined size. 
      PUT (HDRP (PREV_BLKP (bp)), PACK (size, 0));
      PUT (FTRP ((bp)), PACK (size, 0));	//  change  to PUT (FTRP (PREV_BLKP (bp)), PACK (size, 0));

      //set bp to previous block because the header top of the block is now previous  block.
      bp = PREV_BLKP (bp);


      push (bp);

    }

  //else if previous block and next block are free. 
  else
    {				/* Case 4 */

      //remove adjacent free blocks from list .
      pop (PREV_BLKP (bp));
      pop (NEXT_BLKP (bp));

      //add the previous and next blocks size.
      size +=
	GET_SIZE (HDRP (PREV_BLKP (bp))) + GET_SIZE (FTRP (NEXT_BLKP (bp)));

      //set header and footer.
      PUT (HDRP (PREV_BLKP (bp)), PACK (size, 0));
      PUT (FTRP (NEXT_BLKP (bp)), PACK (size, 0));	//change   PUT (FTRP (PREV_BLKP (bp)), PACK (size, 0));

      //set bp to previous block.
      bp = PREV_BLKP (bp);

      //add new block to the free list.  
      push (bp);

    }

  //return pointer to the free block as coalesced 
  return bp;
}

/*************SAME AS GIVEN ************/
/* 
 * Requires:
 *   None.
 *
 * Effects:
 *   Extend the heap with a free block and return that block's address.
 */
static void *
extend_heap (size_t words)
{
  void *bp;
  size_t size;

  /* Allocate an even number of words to maintain alignment. */
  size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

  if ((bp = mem_sbrk (size)) == (void *) -1)
    return (NULL);

  /* Initialize free block header/footer and the epilogue header. */

  PUT (HDRP (bp), PACK (size, 0));	/* Free block header */
  PUT (FTRP (bp), PACK (size, 0));	/* Free block footer */
  PUT (HDRP (NEXT_BLKP (bp)), PACK (0, 1));	/* New epilogue header */

  /* Coalesce if the previous block was free. */
  return (coalesce (bp));
}

/*
 * Requires:
 *   None.
 *
 * Effects:
 *  Go to appropriate index of the map.in that index follow the linked list to find the desired sized free block ,if not found go the the next free block available index and search in that linked list , if found return if not do it till you reach the last index and if found return if not then return null.
 */
static void *
find_fit (size_t asize)
{
/* First fit search */
  void *bp;

  //if there are no free blocks, return null
  if (num_free == 0)
    return NULL;

	// know the index of the hash map.
  int current_index = asize / 25;

	//As there are only 100 blocks if the range is exceeded go to last index.
  if (current_index > 99)
    current_index = 99;
	
   // if the index is less than the first free block index in the map.
  if (current_index < index_start)
    current_index = index_start;
	
	//going till we find the appropriate block.
  for (; current_index < 100; current_index++)
    {
	//just a checker for no making a search too long in the same index.
      int i = 0;
	// if the index is not the last of the map.
      if (current_index != 99)
	{
		// looping till we get the desired block.
	  for (bp = (char *) GET (heap_listp + (current_index * WSIZE));
	       (uintptr_t) bp != 0 && GET_SIZE (HDRP (bp)) > 0 && i < 200;
	       bp = (char *) GET (bp + WSIZE))
	    {
	      if (!GET_ALLOC (HDRP (bp)) && (asize <= GET_SIZE (HDRP (bp))))
		{
			//found the block and returning the pointer.
		  return bp;
		}
	      i++;
	    }
	}
	//block not found yet and this is the last index.
      else
	{
	  for (bp = (char *) GET (heap_listp + (current_index * WSIZE));
	       (uintptr_t) bp != 0 && GET_SIZE (HDRP (bp)) > 0;
	       bp = (char *) GET (bp + WSIZE))
	    {
	      if (!GET_ALLOC (HDRP (bp)) && (asize <= GET_SIZE (HDRP (bp))))
		{
			//found the block and returning the pointer.
		  return bp;
		}
	    }

	}

    }

  //if no fits were found return null
  return NULL;
}

/* 
 * Requires:
 *   "bp" is the address of a free block that is at least "asize" bytes.
 *
 * Effects:
 *   Place a block of "asize" bytes at the start of the free block "bp" and
 *   split that block if the remainder would be at least the minimum block
 *   size. 
 */
static void
place (void *bp, size_t asize)
{
// getting the size of the free block given.
  size_t csize = GET_SIZE (HDRP (bp));
  void *next;
	// if the block is big enough to seperate into two blocks.
  if ((csize - asize) >= (2 * DSIZE))
    {
	//remove the block from the list.
      pop (bp);

	//place the block as allocated.
      PUT (HDRP (bp), PACK (asize, 1));
      PUT (FTRP (bp), PACK (asize, 1));
	
	// setting  the next block .
      next = NEXT_BLKP (bp);

	//setting the remaing size block free.
      PUT (HDRP (next), PACK (csize - asize, 0));
      PUT (FTRP (next), PACK (csize - asize, 0));
	// add the new free block in the list.
      push (next);
    }
  else
    {
	//remove the free block from the list.
      pop (bp);

	//place the block as allocated.
      PUT (HDRP (bp), PACK (csize, 1));
      PUT (FTRP (bp), PACK (csize, 1));

    }
}
/* 
 * Requires:
 *   "bp" is the address of a free block.
 *
 * Effects:
 *   find the appropriate index and places the free block at the starting of the list.
 */

static void
push (void *bp)
{
  int current_index;
  void *next;
  void *current;
  int size;

	//incrementing the number of free blocks by 1.
  num_free++;

	//getting the size/.
  size = GET_SIZE (HDRP (bp));

//going to the appropriate index.
  current_index = size / 25;
  if (current_index > 99)
    current_index = 99;

// if the present block is the first free block or the block is of less size than the present least free block.
  if (index_start > current_index || index_start == 300)
    index_start = current_index;

//checking whether that particular list status.
  current = (char *) GET (heap_listp + (current_index * WSIZE));

//if the list is empty.
  if (current == 0)
    {

      PUT (heap_listp + (current_index * WSIZE), (uintptr_t) bp);

      PUT (bp, 0);

      PUT (bp + WSIZE, 0);
    }

//if the list is not empty.
  else
    {
	//
     next = (char *) GET (current + WSIZE);

	
      PUT (current + WSIZE, (uintptr_t) bp);


      if ((uintptr_t) next != 0)
	PUT (next, (uintptr_t) bp);

	//placing the next and the previous free blocks of the list .
      PUT (bp, (uintptr_t) current);
      PUT (bp + WSIZE, (uintptr_t) next);
    }
}
/* 
 * Requires:
 *   "bp" is the address of a free block.
 *
 * Effects:
 *   find the appropriate index and and the appropriate block and remove from the list.
 */
static void
pop (void *bp)
{
  //decrement the number of free blocks.
  num_free--;

	// getting the prev and next free blocks.
  void *prev = (char *) GET (bp);
  void *next = (char *) GET (bp + WSIZE);

	//getting the size of the block.
  int size = GET_SIZE (HDRP (bp));

  int current_index = size / 25;
  if (current_index > 99)
    current_index = 99;

//if the next and the prev blocks in the list are NULL.
  if (!prev && !next)
    {
      PUT (heap_listp + current_index * WSIZE, 0);
	//if this is the first free block in the entire hash map.
      if (index_start == current_index)
	{
		//if this is not the only free block in the map.
	  if (num_free > 0)
	    {
		//finding the next least sized free block.
	      for (; GET (heap_listp + current_index * WSIZE) == 0;
		   current_index++);
		//setting that as the start index of the first free block.
	      index_start = current_index;
	    }
	  else
	    {
		// no free blocks left in the map/
	      index_start = 300;
	    }
	}
    }
	//if the prev is NULL.
  else if (!prev && next)
    {
      PUT (heap_listp + current_index * WSIZE, (uintptr_t) next);
      PUT (next, 0);
    }
//if the next is null.
  else if (prev && !next)
    {
      PUT ((char *) prev + WSIZE, 0);
    }
//if there are both next and prev.
  else
    {
      PUT ((char *) prev + WSIZE, (uintptr_t) next);
      PUT (next, (uintptr_t) prev);
    }
}

/* 
 * The remaining routines are heap consistency checker routines. 
 */

/*
 * Requires:
 *   "bp" is the address of a block.
 *
 * Effects:
 *   Perform a minimal check on the block "bp".
 */
static void
checkblock (void *bp)
{

  if ((uintptr_t) bp % DSIZE)
    printf ("Error: %p is not doubleword aligned\n", bp);
  if (GET (HDRP (bp)) != GET (FTRP (bp)))
    printf ("Error: header does not match footer\n");
}

/* 
 * Requires:
 *   None.
 *
 * Effects:
 *   Perform a minimal check of the heap for consistency. 
 */
void
checkheap (bool verbose)
{
  void *bp;

  if (verbose)
    printf ("Heap (%p):\n", heap_listp);

  if (GET_SIZE (HDRP (heap_listp)) != DSIZE || !GET_ALLOC (HDRP (heap_listp)))
    printf ("Bad prologue header\n");
  checkblock (heap_listp);

  for (bp = heap_listp; GET_SIZE (HDRP (bp)) > 0; bp = NEXT_BLKP (bp))
    {
      if (verbose)
	printblock (bp);
      checkblock (bp);
    }

  if (verbose)
    printblock (bp);
  if (GET_SIZE (HDRP (bp)) != 0 || !GET_ALLOC (HDRP (bp)))
    printf ("Bad epilogue header\n");
}

/*
 * Requires:
 *   "bp" is the address of a block.
 *
 * Effects:
 *   Print the block "bp".
 */
static void
printblock (void *bp)
{
  bool halloc, falloc;
  size_t hsize, fsize;

  checkheap (false);
  hsize = GET_SIZE (HDRP (bp));
  halloc = GET_ALLOC (HDRP (bp));
  fsize = GET_SIZE (FTRP (bp));
  falloc = GET_ALLOC (FTRP (bp));

  if (hsize == 0)
    {
      printf ("%p: end of heap\n", bp);
      return;
    }

  printf ("%p: header: [%zu:%c] footer: [%zu:%c]\n", bp,
	  hsize, (halloc ? 'a' : 'f'), fsize, (falloc ? 'a' : 'f'));
}


/*
 * The last lines of this file configures the behavior of the "Tab" key in
 * emacs.  Emacs has a rudimentary understanding of C syntax and style.  In
 * particular, depressing the "Tab" key once at the start of a new line will
 * insert as many tabs and/or spaces as are needed for proper indentation.
 */

/* Local Variables: */
/* mode: c */
/* c-default-style: "bsd" */
/* c-basic-offset: 8 */
/* c-continued-statement-offset: 4 */
/* indent-tabs-mode: t */
/* End: */
