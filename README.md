malloc
======

this is an implementation of malloc in c language 

this is optimized for  32 bit and scores 91 .

Achut Nandam    201201169
Bharath Mutyala 201201141
COMP 221 Project 5: Malloc Dynamic Memory Allocator
8/3/2014


DESCRIPTION

this code is made for 32- bit machine. 

We implemented a segregated free list which is a doubly linked list between the free blocks.
What it does is when the initialising function is called,it creates a hashmap in the heap itself.
We created a function add_free_listp which adds an item to the existing list in the same way when something is pushed into a stack, whenever a  free block is added.
In a free block, the first word of the payload contains the address of the previous free block that was pushed into the stack and the second word contains the address of the next free block that would be pushed into the stack.
Functions:

mm_init  : initializes the heap with the program pointer at the end of the prologue.The initialized heap would contain the 4 byte padding ,prologue header ,an array of free list between the prologue header and the prologue footer and the epilogue.


mm_malloc: Given a particular size , it is adjusted in order to include overhead and alignment requests. Then it searches the list for a fit.If no fit was found ,the heap is extended and it is allocated.
find_fit: This function when called , accesses the appropriate index range in the hashmap which depends on the given size.  

push : Adds free block to the list.

Pop : Removes free block to the list.

DESIGN 


We used hash map (chaining), Linked List , idea of Stack. 


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



	Detailed explanation is also given at the starting of every function and before every imp line of code.



TESTING STRATEGY

First we tested for the short1-bal.rep trace file and debugged it by printing text on the screen to know where was the error.
Slowly as it was showing results we started testing with more complex trace files.

Initially we implemented an explicit free list by simply storing the addresses of the previous free block and the next free block in the first and second words of the payload of a particular free block.The result though was nearly the same (around 30).Then we created a free list between the prologue header and the prologue footer.Whenever the heap was extended the address of the newly added free block would be placed in the free list such that they are in ascending order(based on their size) .Then the address of the free block with the largest size that is less than the size of the new free block was stored in the first word of the payload and the address of the free block with the smallest size bigger than that of the free block was stored in the second word of the payload.Once again we did'nt get the result we were hoping we'd get.(It was still pretty much around 40).After some studying and re-examining we implemented a segregated free list which was what has been explained above.And finally we got a satisfactory result and after a lot of changes in the numericals and in the realloc method we managed to improve the score significantly.We then removed the footer from the allocated blocks and ran it again but for reasons unknown to us the score reduced .So we decided to stick with the code with footer in it.
