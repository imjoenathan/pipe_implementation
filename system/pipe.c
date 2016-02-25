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

		newpipe = &piptab[pipnum];
		
		if (newpipe->pipstate == PIP_FREE) {
			newpipe->pipstate = PIP_ONE;
			newpipe->wsem = semcreate(PIPESIZE);
			newpipe->rsem = semcreate(0);
			newpipe->ownerpid = getpid();
			newpipe->writedex = 0;
			newpipe->readdex = 0;
			
			/* Handle creator permission as read or write	*/
			if (!strcmp(mode, "r")) {
				newpipe->ownmode = R_MODE;
				newpipe->rend = getpid();
			} else if (!strcmp(mode, "w")) {
				newpipe->ownmode = W_MODE;
				newpipe->wend = getpid();
			} else {
				kprintf("Argument not supported for pipe creation\n");
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
		kprintf("Bad ppid\n");
		return SYSERR;
	}
	
	struct pipent * pipe;
	pipe = &piptab[pipeid];
	
	if (pipe->pipstate != PIP_ONE) {
		kprintf("Pipe is either free or currently in use\n");
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

	if (isabadppid(pipeid)) {
		kprintf("Bad pipe ID\n");
		return SYSERR;
	}

	struct pipent * pipe;
	pipe = &piptab[pipeid];

	/* Switch statement handling each pipe state			*/
	switch (pipe->pipstate) {
		
		case (PIP_FREE):

			kprintf("Pipe is currently free\n");
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
				return OK;
			}

			kprintf("Calling process cannot close this pipe\n");
			return SYSERR;

		case (PIP_CON):

			/* Check if calling process is reader or writer	*/
			if (pipe->rend == getpid()) {
				
				pid32 wr = pipe->wend;
								

			} else if (pipe->wend == getpid()) {
				//Handle case when writer calls close
			} else {
				kprintf("Calling process is neither the reader nor writer!\n");
				return SYSERR;
			}
		
		case (PIP_USED):
			

		default:
			break;
	}

	return SYSERR;
}
