
void cancel_handler(int sig) {
	if(pthread_cancel_disable) {
		//store cancel as pending
		store_pending();
		return;
	}
	
	setcancelstate(pthread_cancel_disable);
	pthread_sigmask_not allow sigcancel

	pthread_exit(SIGCANCEL);
}


pthread_cancel(pthread_t p) {
	//set handler "cancel_handler" for signal
	init_cancellation();	
	
	if(p = pthread_self())
		pthread_exit();
	pthread_kill(p, SIGCANCEL);
}
