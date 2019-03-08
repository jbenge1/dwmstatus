/*
 * Copy me if you can.
 * by 20h
 */

#define _BSD_SOURCE
#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/statvfs.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>

#include <X11/Xlib.h>

char *tzargentina = "America/Buenos_Aires";
char *tzutc = "UTC";
//char *tzberlin = "America/Phoenix";
char *tzberlin = "Europe/Zurich";

static Display *dpy;

char *
smprintf(char *fmt, ...)
{
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len);
	if (ret == NULL) {
		perror("malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
}

void
settz(char *tzname)
{
	setenv("TZ", tzname, 1);
}

char *
mktimes(char *fmt, char *tzname)
{
	char buf[129];
	time_t tim;
	struct tm *timtm;

	settz(tzname);
	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL)
		return smprintf("");

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		fprintf(stderr, "strftime == 0\n");
		return smprintf("");
	}

	return smprintf("%s", buf);
}

void
setstatus(char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

char *
loadavg(void)
{
	double avgs[3];

	if (getloadavg(avgs, 3) < 0)
		return smprintf("");

	return smprintf("%.2f %.2f %.2f", avgs[0], avgs[1], avgs[2]);
}

char *
readfile(char *base, char *file)
{
	char *path, line[513];
	FILE *fd;

	memset(line, 0, sizeof(line));

	path = smprintf("%s/%s", base, file);
	fd = fopen(path, "r");
	free(path);
	if (fd == NULL)
		return NULL;

	if (fgets(line, sizeof(line)-1, fd) == NULL)
		return NULL;
	fclose(fd);

	return smprintf("%s", line);
}

char *
getbattery(char *base)
{
	char *co, status;
	int descap, remcap;

	descap = -1;
	remcap = -1;

	co = readfile(base, "present");
	if (co == NULL)
		return smprintf("");
	if (co[0] != '1') {
		free(co);
		return smprintf("not present");
	}
	free(co);

	co = readfile(base, "charge_full_design");
	if (co == NULL) {
		co = readfile(base, "energy_full_design");
		if (co == NULL)
			return smprintf("");
	}
	sscanf(co, "%d", &descap);
	free(co);

	co = readfile(base, "charge_now");
	if (co == NULL) {
		co = readfile(base, "energy_now");
		if (co == NULL)
			return smprintf("");
	}
	sscanf(co, "%d", &remcap);
	free(co);

	co = readfile(base, "status");
	if (!strncmp(co, "Discharging", 11)) {
		status = '-';
	} else if(!strncmp(co, "Charging", 8)) {
		status = '+';
	} else {
		status = '?';
	}

	if (remcap < 0 || descap < 0)
		return smprintf("invalid");

	return smprintf("%.0f%%%c", ((float)remcap / (float)descap) * 100, status);
}

char *
gettemperature(char *base, char *sensor)
{
	char *co;

	co = readfile(base, sensor);
	if (co == NULL)
		return smprintf("");
	return smprintf("%02.0f°C", atof(co) / 1000);
}

char *get_freespace(char *mntpt){
    struct statvfs data;
    double total, used = 0;

    if ( (statvfs(mntpt, &data)) < 0){
		fprintf(stderr, "can't get info on disk.\n");
		return("?");
    }
    total = (data.f_blocks * data.f_frsize);
    used = (data.f_blocks - data.f_bfree) * data.f_frsize ;
    return(smprintf("%.0f", (used/total*100)));
}

char *check_internet() 
{
	int sockfd = 0;
  	struct sockaddr_in serv_addr;
 
  	if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
   		return(smprintf("NO"));
      	
    }
 
  	serv_addr.sin_family = AF_INET;
  	serv_addr.sin_port = htons(5000);
  	serv_addr.sin_addr.s_addr = inet_addr("1.1.1.1");
 
  	if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
    {
      	return(smprintf("NO"));
      	
    }
	return(smprintf("ON")); 
}

int
main(void)
{
	char *status;
	char *avgs;
	char *bat;
	char *bat1;
	char *tmar;
	char *tmutc;
	char *tmbln;
	char *t0, *t1, *t2;
	char *rootfs = NULL;
    char *homefs = NULL;
    char pacs[255];
    FILE *fp;
	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwmstatus: cannot open display.\n");
		return 1;
	}
    
	for (;;sleep(20)) {
		avgs = loadavg();
		bat = getbattery("/sys/class/power_supply/BAT0");
		bat1 = getbattery("/sys/class/power_supply/BAT1");
		tmar = mktimes("%H:%M", tzargentina);
		tmutc = mktimes("%H:%M", tzutc);
		tmbln = mktimes("KW %W %a %d %b %H:%M %Z %Y", tzberlin);
		t0 = gettemperature("/sys/class/hwmon/hwmon0", "temp1_input");
		t1 = gettemperature("/sys/class/hwmon/hwmon1", "temp1_input");
		t2 = gettemperature("/sys/class/hwmon/hwmon2", "temp1_input");
		homefs = get_freespace("/home");
        rootfs = get_freespace("/");
        
		fp = fopen("/tmp/packageCount", "r");
        fscanf(fp, "%s", pacs);
        fclose(fp);
		
		status = smprintf("T:%s|%s|%s • B:%s • HOME %s% • ROOT %s% • %s • P %s",
				t0, t1, t2, bat, homefs, rootfs, tmbln, pacs);
		setstatus(status);

		free(t0);
		free(t1);
		free(t2);
		free(avgs);
		free(bat);
		free(bat1);
		free(tmar);
		free(tmutc);
		free(tmbln);
		free(homefs);
		free(rootfs);
		free(status);
	}

	XCloseDisplay(dpy);

	return 0;
}

