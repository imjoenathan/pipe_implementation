#include <xinu.h>

ppid32 popen (const char * mode) {
	
	int32 mask;
	int32 i;
	int32 pipnum;
	struct pipent * newpipe;

	mask = disable();

	for (i = 0; i < NPIPE; i++) {
		
		pipnum = nextpipid;
		nextpipid++;
		if (nextpipid >= NPIPE) {
			nextpipid = 0;
		}

		int i;

		newpipe = &piptab[pipnum];
		
		if (newpipe->pipstate == PIP_FREE) {
			newpipe->pipstate = PIP_ONE;
			newpipe->wsem = semcreate(0);
			newpipe->rsem = semcreate(1);
			newpipe->ownerpid = getpid();
			newpipe->writedex = 0;
			newpipe->readdex = 0;

			for (i = 0; i < PIPESIZE; i++) {
				newpipe->buffer[i] = NULL;
			}

			/* Handle creator permission as read or write	*/
			if (!strcmp(mode, "r")) {
				newpipe->ownmode = R_MODE;
				newpipe->rend = getpid();
			} else if (!strcmp(mode, "w")) {
				newpipe->ownmode = W_MODE;
				newpipe->wend = getpid();
			} else {
				return SYSERR;
			}

			/* Return the new pipes id in the piptab	*/
			restore(mask);
			return pipnum;
		}
	}
	restore(mask);
	return SYSERR;
}

syscall	pjoin (ppid32 pipeid) {

	int mask;
	mask = disable();

	if (isabadppid(pipeid)) {
		return SYSERR;
	}
	
	struct pipent * pipe;
	pipe = &piptab[pipeid];
	
	if (pipe->pipstate != PIP_ONE) {
		return SYSERR;
	}

	/* If pipe owner is in read mode, set currpid to write end and vise verse */
	if (pipe->ownmode == R_MODE) {
		pipe->wend = getpid();
	}
	if (pipe->ownmode == W_MODE) {
		pipe->rend = getpid();
	}

	pipe->pipstate = PIP_CON;

	restore(mask);
	return OK;
}


syscall pclose (ppid32 pipeid) {

	int mask;
	mask = disable();

	if (isabadppid(pipeid)) {
		kprintf("Bad pipe ID\n");
		return SYSERR;
	}

	struct pipent * pipe;
	pipe = &piptab[pipeid];

	/* Switch statement handling each pipe state			*/
	switch (pipe->pipstate) {
		
		case (PIP_FREE):

			restore(mask);
			return SYSERR;
			
		case (PIP_ONE):

			/* Check if calling process is the owner	*/
			if (pipe->ownerpid == getpid()) {
				/* Free the pipe			*/
				semdelete (pipe->wsem);
				semdelete (pipe->rsem);
				pipe->ownerpid = NULL;
				pipe->buffer[0] = NULL;
				pipe->writedex = 0;
				pipe->readdex = 0;
				pipe->rend = NULL;
				pipe->wend = NULL;
				pipe->pipstate = PIP_FREE;
				pipe->ownmode = NULL;
				restore(mask);
				return OK;
			}

			restore(mask);
			return SYSERR;

		case (PIP_CON):

			/* Check if calling process is reader or writer	*/
			if (pipe->rend == getpid()) {

				pid32 wr = pipe->wend;

				pipe->rend = NULL;
				pipe->pipstate = PIP_USED;

				if (isbadpid(wr)) {
					restore(mask);
					return SYSERR;
				}

				struct procent * writ;
				writ  = &proctab[wr];

				/* Check if write end is blocked, if yes then wake	*/
				if (writ->prstate == PR_SUSP) {
					ready(wr);
				}
				if (writ->prstate == PR_WAIT) {
					signal(pipe->wsem);
				}
				restore(mask);
				return OK;	

			} else if (pipe->wend == getpid()) {
				
				pid32 rd = pipe->rend;

				pipe->wend = NULL;
				pipe->pipstate = PIP_USED;

				if (isbadpid(rd)) {
					restore(mask);
					return SYSERR;
				}
				struct procent * read;
				read = &proctab[rd];
				
				/* Check if read end is blocked, if yes then wake	*/
				if (read->prstate == PR_SUSP) {
					ready(rd);
				}
				if (read->prstate == PR_WAIT) {
					signal(pipe->wsem);
				}
				restore(mask);
				return OK;

			} else {
				restore(mask);
				return SYSERR;
			}
		
		case (PIP_USED):
			
			/* Pipe has already been closed on one end, free the pipe	*/
			
			if (pipe->rend == NULL) {
				if (pipe->wend != getpid()) {
					restore(mask);
					return SYSERR;
				}
			} else if (pipe->wend == NULL) {
				if (pipe->rend != getpid()) {
					restore(mask);
					return SYSERR;
				}
			} else {
				restore(mask);
				return SYSERR;
			}

			semdelete (pipe->wsem);
			semdelete (pipe->rsem);
			pipe->ownerpid = NULL;
			pipe->buffer[0] = NULL;
			pipe->writedex = 0;
			pipe->readdex = 0;
			pipe->rend = NULL;
			pipe->wend = NULL;
			pipe->pipstate = PIP_FREE;
			pipe->ownmode = NULL;
			restore(mask);
			return OK;

		default:
			break;
	}
	
	restore(mask);
	return SYSERR;
}


syscall pread(ppid32 pipeid, void * buf, uint32 len) {

	int mask;
	mask = disable();
	
	struct pipent * pipe;

	if (isabadppid(pipeid)) {
		restore(mask);
		return SYSERR;
	}
	
	pipe = &piptab[pipeid];
	
	if (pipe->pipstate != PIP_CON && pipe->pipstate != PIP_USED) {
		restore(mask);
		return SYSERR;
	}

	if (pipe->rend != getpid()) {
		restore(mask);
		return SYSERR;
	}

	int bufferSize = strlen(pipe->buffer);
	int rdex = pipe->readdex;
	
	int charRemain = bufferSize - rdex;	/* Char remaining in buffer	*/

	/* If nothing in buffer to read and write end is closed, return SYSERR	*/
	if (charRemain == -1 && pipe->wend == NULL) {
		restore(mask);
		return SYSERR;
	}

	/* If buffer empty but write end open, suspend until write calls ready	*/
	if (charRemain == 0 && pipe->wend != NULL) {
		wait(pipe->wsem);
	}

	wait(pipe->rsem);

	if (pipe->wend != NULL) {
		suspend(pipe->wend);
	}

	int chRead = 0;
	char * buff = buf;

	/* Read all bytes in the buffer						*/
	while (chRead < len) {
				
		buff[chRead] = pipe->buffer[pipe->readdex]; 
		chRead++;
		pipe->readdex++;

		if (pipe->readdex == PIPESIZE) {
			pipe->readdex = 0;
			break;
		}

	}
	buf = buff;

	if (pipe->wend != NULL) {
		ready(pipe->wend);
	}

	signal(pipe->rsem);

	restore(mask);
	return chRead;
}


syscall pwrite (ppid32 pipeid, void * buf, uint32 len) {

	int mask;
	mask = disable();

	if (isabadppid(pipeid)) {
		restore(mask);
		return SYSERR;
	}

	struct pipent * pipe;
	pipe = &piptab[pipeid];

	if (pipe->pipstate != PIP_CON) {
		restore(mask);
		return SYSERR;
	}

	if (pipe->wend != getpid()) {
		restore(mask);
		return SYSERR;
	}

	if (pipe->rend == NULL) {
		restore(mask);
		return SYSERR;
	}

	char * buff;
	int chWritten = 0;

	wait(pipe->rsem);

	buff = buf;
	
	while (chWritten < len) {
		if (pipe->rend == NULL) {
			restore(mask);
			return SYSERR;
		}

		pipe->buffer[pipe->writedex] = buff[chWritten];
		chWritten++;
		pipe->writedex++;

		if (pipe->writedex == PIPESIZE) {			
			ready(pipe->rend);
			pipe->writedex = 0;
			suspend(getpid());
		}
	}

	if (semcount(pipe->wsem) == -1) {
			signal(pipe->wsem);
	}

	signal(pipe->rsem);

	restore(mask);
	return chWritten;
}
