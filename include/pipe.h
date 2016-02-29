#define	NPIPE		10		/* Max number of processes in system	*/
#define PIPESIZE	256		/* Max size (bytes) of a pipe		*/

#define	PIP_FREE	0		/* Pipe is free				*/
#define	PIP_ONE		1		/* Pipe has one process connected	*/
#define	PIP_CON		2		/* Pipe is currently connected 2 proc	*/
#define PIP_USED	3		/* One end has been closed permanently	*/

#define R_MODE		1		/* Owner is reader			*/
#define	W_MODE		2		/* Owner is writer			*/

struct pipent	{
	uint16	pipstate;		/* State of the pipe			*/
	sid32	wsem;			/* Write semaphore			*/
	sid32	rsem;			/* Mutex semaphore			*/
	pid32	ownerpid;		/* Process ID of pipe creator		*/
	pid32	rend;			/* Read end of the pipe			*/
	pid32	wend;			/* Write end of the pipe		*/
	char 	buffer[PIPESIZE];	/* Buffer that will pass messages	*/
	int32 	writedex;		/* Index of the writer			*/
	int32 	readdex;		/* Index of the reader			*/
	uint16	ownmode;		/* Owner is reader or writer		*/
};

extern 	struct	pipent	piptab[];	/* Table of all pipes			*/
extern	ppid32	nextpipid;		/* Points to next pipe in table		*/

#define isabadppid(p)	((p) < 0 || (p) >= NPIPE)
