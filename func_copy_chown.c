// Copy-Move file up to 2GB
static bool internal_bu_copy_file(const char *tmp_filepath, const char *target_filepath,bool overwrite_if_exist, bool move_file){

	int fd_src = -1;
	int fd_out = -1;
	bool status = false;
	ssize_t receive_bytes = 0;

	if((fd_src = open(tmp_filepath, O_RDONLY)) < 0 ){
		log_error("Cannot open file %s (errno %d)", tmp_filepath, errno);
		goto _free;
	}
	struct stat stat_src;
	if(fstat(fd_src, &stat_src) < 0){
		log_error("Cannot get stat file %s (errno %d)", tmp_filepath, errno);
		goto _free;
	}
	if(overwrite_if_exist){
		if (unlink(target_filepath) < 0 && errno != ENOENT) {
			log_error("Cannot unlink file %s (errno %d)", target_filepath, errno);
			goto _free;
		}
	}
	if((fd_out = open(target_filepath, O_WRONLY | O_CREAT,stat_src.st_mode)) < 0 ){
		log_error("Cannot create file %s (errno %d)", target_filepath, errno);
		goto _free;
	}
	if((receive_bytes = sendfile(fd_out, fd_src, 0, (size_t) stat_src.st_size) ) < 0){
		log_error("Cannot write file %s (errno %d)", target_filepath, errno);
		goto _free;
	}
	if( receive_bytes != stat_src.st_size){
		log_error("Write not all data, file %s", target_filepath);
		goto _free;
	}
	if( move_file ){
		if (unlink(tmp_filepath) < 0 ) {
			log_error("Cannot unlink file %s (errno %d)", tmp_filepath, errno);
			goto _free;
		}
	}
	status = true;
_free:
	if(fd_src > 0) close(fd_src);
	if(fd_out > 0) close(fd_out);

	return status;
}
static bool internal_bu_set_owner_file(const char *owner_name, const char *filepath){

	struct passwd pwd;
	struct passwd *result;
	char *buf_pwnam=NULL;
	long int buf_size_pwnam;
	int ret_pwnam;
	bool status = false;

	buf_size_pwnam = sysconf(_SC_GETPW_R_SIZE_MAX);
	if (buf_size_pwnam == -1)
		buf_size_pwnam = BU_SET_OWNER_BUF_SIZE_DEF;


	buf_pwnam = malloc((size_t)buf_size_pwnam);
        if (buf_pwnam == NULL){
		log_error("Error malloc %d (errno %d)", (int)buf_size_pwnam, errno);
		return false;
	}

	ret_pwnam = getpwnam_r(owner_name, &pwd, buf_pwnam, (size_t)buf_size_pwnam, &result);
        if (result == NULL){
		log_error("Not found user %s (errno %d)", owner_name, ret_pwnam);
		goto _free;
	}

        if (chown(filepath, pwd.pw_uid, 0) == -1) {
		log_error("Not chown file %s (errno %d)", filepath, errno);
		goto _free;
        }
	status = true;
_free:
	if(buf_pwnam) free(buf_pwnam);

	return status;
}
