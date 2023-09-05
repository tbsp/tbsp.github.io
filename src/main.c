#include <stdio.h>
#include <time.h>
/*#include <errno.h>*/

#define STRMEM 4096 * 96
#define GLOMEM 100
#define LEXMEM 500
#define LOGMEM 4096 * 4
#define ITEMS 128 /* Note: Extended to support my insane tabletop collection */

#define JORNAL_HOST_LENGTH 24

#define NAME "wastingmoves"
#define DOMAIN "https://wastingmoves.com/"
#define LOCATION "Vancouver, Canada"

typedef struct Block {
	int len;
	char data[STRMEM];
} Block;

typedef struct List {
	int len, routes;
	char *name, *keys[ITEMS], *vals[ITEMS], *url[ITEMS];
} List;

typedef struct Term {
	int body_len, children_len, incoming_len, outgoing_len, logs_len, events_len;
	char *name, *host, *bref, *type, *filename, *date, *date_from, *date_last, *caption;
	char *body[ITEMS];
	struct List link;
	struct Term *parent;
	struct Term *children[ITEMS];
	struct Term *incoming[ITEMS];
} Term;

typedef struct Log {
	int len;
	char *date, *name, *pict;
	Term *term;
} Log;

typedef struct Glossary {
	int len;
	List lists[GLOMEM];
} Glossary;

typedef struct Lexicon {
	int len;
	Term terms[LEXMEM];
} Lexicon;

typedef struct Journal {
	int len;
	Log logs[LOGMEM];
} Journal;

#pragma mark - Helpers

/* clang-format off */

int	  cisp(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; } /* char is space */
int   cial(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }/* char is alpha */
int   cinu(char c) { return c >= '0' && c <= '9'; } /* char is num */
int   clca(int c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; } /* char to lowercase */
int   cuca(char c) { return c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c; } /* char to uppercase */
int   spad(char *s, char c) { int i = 0; while(s[i] && s[i] == c && s[++i]) { ; } return i; } /* string count padding */
int   slen(char *s) { int i = 0; while(s[i] && s[++i]) { ; } return i; } /* string length */
char *suca(char *s) { int i = 0; char c; while((c = s[i])) s[i++] = cuca(c);return s; } /* string to uppercase */
char *slca(char *s) { int i = 0; char c; while((c = s[i])) { s[i++] = clca(c); } return s; } /* string to lowercase */
int   scmp(char *a, char *b) { int i = 0; while(a[i] == b[i]) if(!a[i++]) return 1; return 0; } /* string compare */
char *scpy(char *src, char *dst, int len) { int i = 0; while((dst[i] = src[i]) && i < len - 2) i++; dst[i + 1] = '\0'; return dst; } /* string copy */
int   sint(char *s, int len) { int n = 0, i = 0; while(s[i] && i < len && (s[i] >= '0' && s[i] <= '9')) n = n * 10 + (s[i++] - '0'); return n; } /* string to num */
char *scsw(char *s, char a, char b) { int i = 0; char c; while((c = s[i])) s[i++] = c == a ? b : c; return s; } /* string char swap */
int   sian(char *s) { int i = 0; char c; while((c = s[i++])) if(!cial(c) && !cinu(c) && !cisp(c)) return 0; return 1; } /* string is alphanum */
int   scin(char *s, char c) { int i = 0; while(s[i]) if(s[i++] == c) return i - 1; return -1; } /* string char index */
int   ssin(char *s, char *ss) { int a = 0, b = 0; while(s[a]) { if(s[a] == ss[b]) { if(!ss[b + 1]) return a - b; b++; } else b = 0; a++; } return -1; } /* string substring index */
char *strm(char *s) { char *end; while(cisp(*s)) s++; if(*s == 0) return s; end = s + slen(s) - 1; while(end > s && cisp(*end)) end--; end[1] = '\0'; return s; }
int   surl(char *s) { return ssin(s, "://") >= 0 || ssin(s, "../") >= 0; } /* string is url */
char *sstr(char *src, char *dst, int from, int to) { int i; char *a = (char *)src + from, *b = (char *)dst; for(i = 0; i < to; i++) b[i] = a[i]; dst[to] = '\0'; return dst; }
char *ccat(char *dst, char c) { int len = slen(dst); dst[len] = c; dst[len + 1] = '\0'; return dst; }
char *scat(char *dst, const char *src) { char *ptr = dst + slen(dst); while(*src) { *ptr++ = *src++; } *ptr = '\0'; return dst; }
/* clang-format on */

#pragma mark - Core

int
error(char *msg, char *val)
{
	printf("Error: %s(%s)\n", msg, val);
	return 0;
}

int
errorid(char *msg, char *val, int id)
{
	printf("Error: %s:%d(%s)\n", msg, id, val);
	return 0;
}

#pragma mark - Block

char *
push(Block *b, char *s)
{
	int i = 0, o = b->len;
	while(s[i])
		b->data[b->len++] = s[i++];
	b->data[b->len++] = '\0';
	return &b->data[o];
}

#pragma mark - List

List *
makelist(List *l, char *name)
{
	l->len = 0;
	l->routes = 0;
	l->name = slca(name);
	return l;
}

List *
findlist(Glossary *glo, char *name)
{
	int i;
	for(i = 0; i < glo->len; ++i)
		if(scmp(name, glo->lists[i].name))
			return &glo->lists[i];
	return NULL;
}

#pragma mark - Term

Term *
maketerm(Term *t, char *name)
{
	t->body_len = 0;
	t->children_len = 0;
	t->incoming_len = 0;
	t->outgoing_len = 0;
	t->logs_len = 0;
	t->events_len = 0;
	t->name = slca(name);
	return t;
}

Term *
findterm(Lexicon *lex, char *name)
{
	int i;
	scsw(slca(name), '_', ' ');
	for(i = 0; i < lex->len; ++i)
		if(scmp(name, lex->terms[i].name))
			return &lex->terms[i];
	return NULL;
}

char *
statusterm(Term *t)
{
	if(t->body_len < 1)
		return "stub";
	if(t->incoming_len < 1)
		return "orphan";
	if(t->outgoing_len < 1)
		return "deadend";
	return "";
}

#pragma mark - Log

Log *
makelog(Log *l, char *date)
{
	l->date = date;
	return l;
}

Log *
finddiary(Journal *jou, Term *t, int deep)
{
	int i;
	for(i = 0; i < jou->len; ++i)
		if(jou->logs[i].term == t)
			return &jou->logs[i];
	if(deep)
		for(i = 0; i < t->children_len; ++i)
			return finddiary(jou, t->children[i], 0);
	return NULL;
}

#pragma mark - File
FILE *
getfile(char *dir, char *filename, char *ext, char *op)
{
	char filepath[1024];
	filepath[0] = '\0';
	scat(filepath, dir);
	scat(filepath, filename);
	scat(filepath, ext);
	scat(filepath, "\0");
	return fopen(filepath, op);
	/*FILE *dfile;
	dfile = fopen(filepath, op);
	if(dfile == NULL) {
		printf("fopen failed, %s errno = %d\n", filepath, errno);
	} else {
		printf("fopen succeeded\n");
	}
	return dfile;*/
}

#pragma mark - Time

float
clockoffset(clock_t start)
{
	return (((double)(clock() - start)) / CLOCKS_PER_SEC) * 1000;
}

int
ispl(int i, int index, int len)
{
	int c = 0;
	char src[128], dst[32];
	sprintf(src, "%d", i);
	dst[0] = '\0';
	while(c < len) {
		dst[c] = src[index + c];
		c++;
	}
	dst[len] = '\0';
	return sint(dst, len + 1);
}

time_t
ymdstrtime(int y, int m, int d)
{
	struct tm stime;
	stime.tm_year = y - 1900;
	stime.tm_mday = d;
	stime.tm_mon = m - 1;
	stime.tm_hour = 0;
	stime.tm_min = 0;
	stime.tm_sec = 1;
	stime.tm_isdst = -1;
	return mktime(&stime);
}

time_t
itotime(int i)
{
	int year, month, day;
	year = ispl(i, 0, 2) + 2000;
	month = ispl(i, 2, 2);
	day = ispl(i, 4, 2);
	return ymdstrtime(year, month, day);
}

void
fpRFC2822(FILE *f, time_t t, int time)
{
	struct tm *tm = localtime(&t);
	char *days[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	fprintf(f,
		"%s, %02d %s %d%s",
		days[tm->tm_wday],
		tm->tm_mday,
		months[tm->tm_mon],
		tm->tm_year + 1900,
		time ? " 00:00:00 +0100" : "");
}

#pragma mark - Fprint

int
fptemplatelink(FILE *f, Lexicon *lex, Term *t, char *s)
{
	int split = scin(s, ' ');
	char target[256], name[256];
	/* find target and name */
	if(split == -1)
		scpy(sstr(s, target, 0, slen(s)), name, 256);
	else {
		sstr(s, target, 0, split);
		sstr(s, name, split + 1, slen(s) - split);
	}
	/* output */
	if(surl(target)) {
		if(f)
			fprintf(f, "<a href='%s' target='_blank'>%s</a>", target, name);
	} else {
		Term *tt = findterm(lex, target);
		if(!tt)
			return error("Unknown link", target);
		if(f)
			fprintf(f, "<a href='%s.html'>{%s}</a>", tt->filename, name);
		else {
			tt->incoming[tt->incoming_len++] = t;
			t->outgoing_len++;
		}
	}
	return 1;
}

int
fplist(FILE *f, Glossary *glo, char *target)
{
	int j;
	List *l = findlist(glo, target);
	if(!l)
		return error("Unknown list", target);
	fprintf(f, "<h3>%s</h3>\n", l->name);
	fputs("<ul>\n", f);
	for(j = 0; j < l->len; ++j) {
		if(l->vals[j] && l->keys[j] && l->url[j])
			fprintf(f, "<li><a href='%s' target='_blank'>%s</a> - %s</li>\n", l->url[j], l->keys[j], l->vals[j]);
		else if(l->vals[j] && l->keys[j] && !l->url[j])
			fprintf(f, "<li><b>%s</b>: %s</li>", l->keys[j], l->vals[j]);
		else if(!l->vals[j] && l->keys[j] && l->url[j])
			fprintf(f, "<li><a href='%s' target='_blank'>%s</a></li>\n", l->url[j], l->keys[j]);
		else if(!l->keys[j] && l->vals[j] && !l->url[j])
			fprintf(f, "<li>%s</li>\n", l->vals[j]);
	}
	fputs("</ul>\n", f);
	l->routes++;
	return 1;
}

int
fppict(FILE *f, char *filename, char *caption, int header, int link)
{
	FILE *img;
	char path[64], realpath[64], name[64], ext[16], dst[16], buf[32], srcset[256];
	int sizes[3] = {240, 680, 900};
	int split, i, count = 0, isVideo = 0;
	name[0] = '\0';
	scpy(filename, name, slen(filename) + 1);
	split = scin(name, '/');
	path[0] = '\0';
	buf[0] = '\0';
	scat(path, "../media/");
	while(split != -1) {
		scat(path, sstr(name, buf, 0, split + 1));
		sstr(name, name, split + 1, slen(name) - split);
		split = scin(name, '/');
	}
	split = scin(name, '.');
	if(split > 0) {
		sstr(name, ext, split, slen(name) - split);
		sstr(name, name, 0, split);
	} else {
		ext[0] = '\0';
		scat(ext, ".jpg");
	}
	if(scmp(ext, ".mp4"))
		isVideo = 1;
	img = getfile(path, name, ext, "r");
	if(!img) {
		if(!header && link)
			error("Couldn't open image ", scat(scat(path, name), ext));
		return 0;
	}
	fclose(img);
	realpath[0] = '\0';
	scpy(path, realpath, slen(path) + 1);
	sstr(realpath, realpath, 3, slen(path));
	if(!isVideo) {
		srcset[0] = '\0';
		buf[0] = '\0';
		for(i = 0; i < 3; i++) {
			dst[0] = '\0';
			scat(dst, "-");
			sprintf(buf, "%d", sizes[i]);
			scat(dst, buf);
			scat(dst, ".png");
			img = getfile(path, name, dst, "r");
			if(!img) {
				printf("\n-> %s\%s%s", path, name, ext);
				printf("Couldn't open image %s%s%s\n", path, name, dst);
				continue;
			}
			if(count > 0)
				scat(srcset, ", ");
			scat(srcset, realpath);
			scat(srcset, name);
			scat(srcset, dst);
			scat(srcset, " ");
			scat(srcset, buf);
			scat(srcset, "w");
			fclose(img);
			count++;
		}
	}
	if(header)
		fputs("<header>\n", f);
	fputs("<figure>\n", f);
	if(isVideo)
		fprintf(f, "<video autoplay loop src='%s%s%s' type='video/mp4'></video>\n", realpath, name, ext);
	else {
		if(count > 1) {
			fprintf(f, "<a href='%s%s%s'>\n", realpath, name, ext);
			fprintf(f, "<img srcset='%s' sizes='(max-width: 480px) 240px, 680px' src='%s%s-240.png' alt='' loading='lazy'>\n", srcset, realpath, name);
			fputs("</a>\n", f);
		} else
			fprintf(f, "<img src='media/%s' width='auto' alt='' loading='lazy'/>\n", filename);
	}
	if(caption)
		fprintf(f, "<figcaption>%s</figcaption>\n", caption);
	fputs("</figure>\n", f);
	if(header)
		fputs("</header>\n", f);
	return 1;
}

int
fpinclude(FILE *f, char *file, int type)
{
	int lines = 0;
	char c;
	char path[128], ext[64], fullpath[128];
	FILE *fp;
	path[0] = '\0';
	ext[0] = '\0';
	switch(type) {
	case 1:
		scat(path, "inc/text/");
		scat(ext, ".txt");
		break;
	case 2:
		scat(path, "inc/code/");
		break;
	default:
		scat(path, "inc/html/");
		scat(ext, ".html");
		break;
	}
	fullpath[0] = '\0';
	scat(fullpath, path);
	scat(fullpath, file);
	if(type == 0 || type == 1)
		scat(fullpath, ext);
	fp = getfile(path, file, ext, "r");
	if(!fp)
		return error("Missing include", fullpath);
	if(type == 1)
		fputs("<article>\n<p>", f);
	else if(type == 2)
		fputs("<pre>\n", f);
	while((c = fgetc(fp)) != EOF) {
		if(type || type == 2) {
			if(c == '<')
				fputs("&lt;", f);
			else if(c == '>')
				fputs("&gt;", f);
			else if(c == '&')
				fputs("&amp;", f);
			else
				fputc(c, f);
		} else
			fputc(c, f);
		if(c == '\n') {
			if(type == 1)
				fputs("<br>", f);
			lines++;
		}
	}
	fclose(fp);
	if(type == 1)
		fputs("</p>\n</article>\n", f);
	else if(type == 2)
		fputs("</pre>\n", f);
	return 1;
}

int
fptable(FILE *f, char *target)
{
	int i;
	char buf[512];
	buf[0] = '\0';
	fputs("<tr>", f);
	for(i = 0; i < slen(target); i++) {
		char c = target[i];
		if(c != '|')
			if(c == '_')
				ccat(buf, ' ');
			else
				ccat(buf, c);
		else {
			fprintf(f, "<td>%s</td>", buf);
			buf[0] = '\0';
		}
	}
	fprintf(f, "<td>%s</td></tr>", buf);
	return 1;
}

int
fpmarbles(FILE *f, char *target)
{
	int total, marble, remaining, year, month, day;
	float percentage, used;
	char buf_year[32], buf_month[16], buf_day[16];
	time_t birth;
	year = sint(sstr(target, buf_year, 0, 4), 4);
	month = sint(sstr(target, buf_month, 5, 2), 2);
	day = sint(sstr(target, buf_day, 8, 2), 2);
	birth = ymdstrtime(year, month, day);
	total = 79 * 52;
	marble = (time(NULL) - birth) / (60 * 60 * 24) / 7;
	remaining = total - marble;
	percentage = (double)remaining / (double)total * 100;
	used = 100 - percentage;
	fprintf(f, "<p><b>Marbles:</b> %d<br /><b>Marbles used:</b> %d - %.2f%%<br /><b>Marbles remaining:</b> %d - %.2f%%</p>\n", total, marble, used, remaining, percentage);
	fputs("<div style='background-color:gray;'>", f);
	fprintf(f, "<div style='width:calc(%.2f%% - 10px);height:100%%;padding:2px 2px 2px 8px;color:white;background-color:black;'>%.2f%%</div>", used, used);
	fputs("</div>", f);
	return 1;
}

void
fpmodule(FILE *f, Glossary *glo, char *s)
{
	int split = scin(s, ' ');
	char cmd[256], target[256];
	sstr(s, cmd, 1, split - 1);
	sstr(s, target, split + 1, slen(s) - split);
	if(scmp(cmd, "list"))
		fplist(f, glo, target);
	else if(scmp(cmd, "html"))
		fpinclude(f, target, 0);
	else if(scmp(cmd, "text"))
		fpinclude(f, target, 1);
	else if(scmp(cmd, "code"))
		fpinclude(f, target, 2);
	else if(scmp(cmd, "img")) {
		int split2 = scin(target, ' ');
		if(split2 > 0) {
			char filename[256], caption[256];
			sstr(target, filename, 0, split2);
			sstr(target, caption, split2 + 1, slen(target) - split2);
			fppict(f, filename, caption, 0, 1);
		} else
			fppict(f, target, NULL, 0, 1);
	} else if(scmp(cmd, "marbles")) {
		fpmarbles(f, target);
	} else
		printf("Warning: missing template mod: %s\n", s);
}

void
fptemplate(FILE *f, Glossary *glo, Lexicon *lex, Term *t, char *s)
{
	int i = 0, capture = 0, len;
	char buf[1024];
	len = slen(s);
	for(i = 0; i < len; ++i) {
		char c = s[i];
		if(c == '}') {
			capture = 0;
			if(buf[0] == '^' && f)
				fpmodule(f, glo, buf);
			else if(buf[0] != '^')
				fptemplatelink(f, lex, t, buf);
		}
		if(capture)
			ccat(buf, c);
		else if(c != '{' && c != '}' && f)
			fputc(c, f);
		if(c == '{') {
			capture = 1;
			buf[0] = '\0';
		}
	}
}

void
fptag(FILE *f, Glossary *glo, Lexicon *lex, Term *t, char *s)
{
	int i = 0, len, tag = 0, split = 0;
	char buf[1024], new[1024], tmp[1024], tags[64];
	buf[0] = '\0';
	len = slen(s);
	if(s[0] == '#')
		tag = 0;
	else if(s[0] == '"' && s[len - 1] == '"')
		tag = 1;
	else if(s[0] == '|' && s[len - 1] == '|')
		tag = 2;
	else if(s[0] == '-' && s[1] == ' ')
		tag = 3;
	else if(s[0] == '+' && s[1] == ' ')
		tag = 4;
	else if(s[0] == '>' && s[1] == ' ')
		tag = 5;
	else if(s[0] == '^' && s[len - 1] == '^')
		tag = 6;
	else if(s[0] != '{' && s[0] != '<')
		tag = 7;
	else
		tag = -1;
	switch(tag) {
	case 0:
		while(s[i] == '#')
			++i;
		if(s[i] == ' ') {
			char c[16];
			scat(buf, "<h");
			sprintf(c, "%d", i);
			scat(buf, c);
			scat(buf, ">");
			scat(buf, sstr(s, new, i + 1, len));
			scat(buf, "</h");
			scat(buf, c);
			scat(buf, ">\n");
		}
		break;
	case 1:
		scat(buf, "<q>");
		scat(buf, sstr(s, new, 1, len - 2));
		scat(buf, "</q>\n");
		break;
	case 2:
		new[0] = '\0';
		scpy(s, tmp, len + 1);
		scat(buf, "<tr>\n");
		while(split != -1) {
			if(split > 0) {
				scat(buf, "<td>");
				sstr(tmp, new, 1, split - 1);
				while(new[slen(new) - 1] == ' ')
					sstr(tmp, new, 1, slen(new) - 1);
				scat(buf, new);
				scat(buf, "</td>\n");
			}
			sstr(tmp, tmp, split + 1, slen(tmp) - split);
			split = scin(tmp, '|');
		}
		scat(buf, "</tr>\n");
		break;
	case 3:
		scat(buf, "<cite>");
		scat(buf, sstr(s, new, 2, len - 1));
		scat(buf, "</cite>\n");
		break;
	case 4:
		scat(buf, "<li>");
		scat(buf, sstr(s, new, 2, len - 1));
		scat(buf, "</li>\n");
		break;
	case 5:
		scat(buf, "<pre>\n");
		scat(buf, sstr(s, new, 2, len - 1));
		scat(buf, "</pre>\n");
		break;
	case 6:
		scat(buf, "<p class='large'>");
		scat(buf, sstr(s, new, 1, len - 2));
		scat(buf, "</p>\n");
		break;
	case 7:
		scat(buf, "<p>");
		scat(buf, s);
		scat(buf, "</p>\n");
		break;
	default:
		scpy(s, buf, len + 1);
		break;
	}
	len = slen(buf);
	for(i = 0; i < len; ++i) {
		char c = buf[i];
		if(c == '*' || c == '~' || c == '`') {
			new[0] = '\0';
			scat(new, sstr(buf, tmp, 0, i));
			sstr(buf, tmp, i + 1, len - i + 1);
			split = scin(tmp, c);
			if(split > 0) {
				switch(c) {
				case '~': scpy("i", tags, 2); break;
				case '`': scpy("code", tags, 5); break;
				default: scpy("b", tags, 2); break;
				}
				scat(new, "<");
				scat(new, tags);
				scat(new, ">");
				scat(new, sstr(buf, tmp, i + 1, split));
				scat(new, "</");
				scat(new, tags);
				scat(new, ">");
				scat(new, sstr(buf, tmp, i + 2 + split, len - split));
				len = slen(new);
				scpy(new, buf, len + 1);
			}
		}
	}
	fptemplate(f, glo, lex, t, buf);
}

void
fpbodypart(FILE *f, Glossary *glo, Lexicon *lex, Term *t)
{
	int i, list = 0, table = 0, code = 0, formatcode = 1, len;
	for(i = 0; i < t->body_len; ++i) {
		char *line = t->body[i];
		len = slen(line);
		if(line[0] == '+' && line[1] == ' ' && f && formatcode) {
			if(!list) {
				fputs("<ul>\n", f);
				list = 1;
			}
		} else if(list) {
			fputs("</ul>\n", f);
			list = 0;
		}
		if(line[0] == '|' && line[len - 1] == '|' && f && formatcode) {
			if(!table) {
				fputs("<table>\n", f);
				table = 1;
			}
		} else if(table) {
			fputs("</table>\n", f);
			table = 0;
		}
		if(line[0] == '`' && line[1] == '`' && f && len < 4) {
			if(line[2] == '`')
				formatcode = !formatcode;
			if(!code) {
				fputs("<pre>\n", f);
				code = 1;
			} else {
				fputs("</pre>\n", f);
				code = 0;
			}
			continue;
		}
		if(formatcode)
			fptag(f, glo, lex, t, line);
		else {
			fptemplate(f, glo, lex, t, line);
			fputs("\n", f);
		}
		if(i == t->body_len - 1 && list && f)
			fputs("</ul>\n", f);
		if(i == t->body_len - 1 && table && f)
			fputs("</table>\n", f);
		if(i == t->body_len - 1 && code && f) {
			fputs("</pre>\n", f);
			continue;
		}
	}
}

void
fpbody(FILE *f, Glossary *glo, Lexicon *lex, Term *t)
{
	fprintf(f, "<h1>%s</h1>\n", t->bref);
	fpbodypart(f, glo, lex, t);
}

void
fpportal(FILE *f, Glossary *glo, Lexicon *lex, Term *t, int text, int img)
{
	int i;
	char caption[256], imgpath[256];
	for(i = 0; i < t->children_len; ++i) {
		Term *tc = t->children[i];
		if(tc->name == t->name || (tc->type && scmp(tc->type, "hidden")))
			continue;
		fprintf(f, "<h2><a href='%s.html'>%s</a></h2>\n", tc->filename, tc->name);
		caption[0] = '\0';
		if(tc->date) {
			scat(caption, tc->date);
			scat(caption, " | ");
		}
		scat(caption, tc->bref);
		if(img) {
			imgpath[0] = '\0';
			scat(imgpath, "headers/");
			scat(imgpath, tc->filename);
			fppict(f, imgpath, caption, 0, 0);
		} else
			fprintf(f, "<sub>%s</sub>\n", tc->bref);
		if(text)
			fpbodypart(f, glo, lex, tc);
	}
}

void
fpnavsub(FILE *f, Term *t, Term *target)
{
	int i;
	fputs("<ul>\n", f);
	for(i = 0; i < t->children_len; ++i) {
		Term *tc = t->children[i];
		if(tc->name == t->name)
			continue; /* Paradox */
		if(tc->type && scmp(tc->type, "hidden"))
			continue;
		if(tc->name == target->name)
			fprintf(f, "<li><b><a href='%s.html'>%s/</a></b></li>\n", tc->filename, tc->name);
		else
			fprintf(f, "<li><a href='%s.html'>%s</a></li>\n", tc->filename, tc->name);
	}
	fputs("</ul>\n", f);
}

void
fpnav(FILE *f, Term *t)
{
	if(!t->parent)
		error("Missing parent", t->name);
	if(!t->parent->parent)
		error("Missing parent", t->parent->name);
	fputs("<nav>\n", f);
	fputs("<a href='index.html'><img src='media/icon/wastingmoves.svg' alt='" NAME "' height='100px' width='100px' /></a>\n", f);
	if(t->parent->parent->name == t->parent->name)
		fpnavsub(f, t->parent->parent, t);
	else
		fpnavsub(f, t->parent->parent, t->parent);
	if(t->parent->parent->name != t->parent->name)
		fpnavsub(f, t->parent, t);
	if(t->parent->name != t->name)
		fpnavsub(f, t, t);
	fputs("</nav>\n", f);
}

void
fplinks(FILE *f, Term *t)
{
	int i;
	if(t->link.len < 1)
		return;
	fputs("<h3>Links</h3><ul>\n", f);
	for(i = 0; i < t->link.len; ++i)
		fprintf(f, "<li><a href='%s' target='_blank'>%s</a></li>\n", t->link.vals[i], t->link.keys[i]);
	fputs("</ul>", f);
}

void
fpincoming(FILE *f, Term *t)
{
	int i;
	if(t->incoming_len < 1)
		return;
	fputs("<p>\n", f);
	fprintf(f, "<i>incoming(%d)</i> ", t->incoming_len);
	for(i = 0; i < t->incoming_len; ++i)
		fprintf(f, "| <a href='%s.html'>%s</a> ", t->incoming[i]->filename, t->incoming[i]->name);
	fputs("\n</p>\n", f);
}

void
fpjournal(FILE *f, Journal *jou)
{
	int i;
	char capt[256];
	Log l;
	for(i = 0; i < jou->len; ++i) {
		l = jou->logs[i];
		fprintf(f, "<h3>%s</h3>\n", l.date);
		capt[0] = '\0';
		if(l.term) {
			scat(capt, "<a href='");
			scat(capt, l.term->filename);
			scat(capt, ".html'>");
			scat(capt, l.term->name);
			scat(capt, "</a>");
			scat(capt, " | ");
		}
		scat(capt, l.name);
		fppict(f, l.pict, capt, 0, 1);
	}
}

void
fpindexsub(FILE *f, Term *t, int depth)
{
	int i;
	fprintf(f, "<li><a href='%s.html'>%s</a> <i>%s</i></li>\n", t->filename, t->name, statusterm(t));
	if(t->children_len < 1)
		return;
	fputs("<ul>\n", f);
	for(i = 0; i < t->children_len; ++i)
		if(!scmp(t->children[i]->name, t->name))
			fpindexsub(f, t->children[i], depth++);
	fputs("</ul>\n", f);
}

void
fpindex(FILE *f, Lexicon *lex)
{
	int i, sends = 0, stubs = 0, orphans = 0, deadends = 0;
	for(i = 0; i < lex->len; ++i) {
		Term *t = &lex->terms[i];
		sends += t->incoming_len;
		if(t->body_len < 1)
			stubs++;
		if(t->incoming_len < 1)
			orphans++;
		if(t->outgoing_len < 1)
			deadends++;
	}
	fprintf(f, "<p>This wiki hosts journal logs recorded on %d lexicon terms, connected by %d inbound links. It is a living document in which %d stubs, %d orphans and %d deadends still remain.</p>\n", lex->len, sends, stubs, orphans, deadends);
	fputs("<ul>\n", f);
	fpindexsub(f, &lex->terms[0], 0);
	fputs("</ul>\n", f);
}

void
fphtml(FILE *f, Glossary *glo, Lexicon *lex, Term *t, Journal *jou)
{
	char sub[256], imgpath[256];
	time_t now;
	time(&now);
	imgpath[0] = '\0';
	scat(imgpath, "headers/");
	scat(imgpath, t->filename);
	fputs("<!DOCTYPE html>\n<html lang='en'>\n", f);
	fputs("<head>\n", f);
	fprintf(f, "<meta charset='utf-8'>\n"
			   "<meta name='description' content='%s' />\n"
			   "<meta name='author' content='tbsp' />\n"
			   "<meta name='viewport' content='width=device-width, initial-scale=1' />\n"
			   "<link rel='shortcut icon' type='image/png' href='media/icon/wastingmoves_favicon.png' />\n"
			   "<link rel='stylesheet' type='text/css' href='links/main.css' />\n"
			   "<title>%s: %s</title>\n"
			   "<meta property='og:title' content='%s' />\n"
			   "<meta property='og:type' content='website' />\n"
			   "<meta property='og:description' content='%s' />\n"
			   "<meta property='og:site_name' content='wastingmoves' />\n"
			   "<meta property='og:url' content='https://wastingmoves.com/%s.html' />\n"
			   "<meta property='og:image' content='https://wastingmoves.com/media/%s-240.png' />\n",
		t->bref,
		NAME,
		t->name,
		t->name,
		t->bref,
		t->filename,
		imgpath);
	fputs("</head>\n", f);
	fputs("<body>\n", f);
	fpnav(f, t);
	sub[0] = '\0';
	if(t->date && t->caption) {
		scat(sub, t->date);
		scat(sub, " | ");
		scat(sub, t->caption);
	} else if(t->date)
		scat(sub, t->date);
	else if(t->caption)
		scat(sub, t->caption);
	fppict(f, imgpath, sub, 1, 0);
	fputs("<main>\n", f);
	fpbody(f, glo, lex, t);
	if(t->type) {
		if(scmp(t->type, "text_portal"))
			fpportal(f, glo, lex, t, 1, 0);
		if(scmp(t->type, "img_portal"))
			fpportal(f, glo, lex, t, 0, 1);
		if(scmp(t->type, "full_portal"))
			fpportal(f, glo, lex, t, 1, 1);
	} else
		fpportal(f, glo, lex, t, 0, 0);
	if(scmp(t->name, "404"))
		fpindex(f, lex);
	if(scmp(t->name, "journal"))
		fpjournal(f, jou);
	fplinks(f, t);
	fpincoming(f, t);
	fputs("</main>\n", f);
	fputs("<footer>\n", f);
	fprintf(f, "<img src='media/icon/arrow_up.svg' /> <a href='#'>Back to top</a> | last edit: <em>%s</em>\n", ctime(&now));
	fputs("<hr />\n", f);
	fputs("<section>\n", f);
	fputs("<a href='https://creativecommons.org/licenses/by-nc-sa/4.0'><img src='media/icon/cc.svg' /></a>\n", f);
	/*fputs("<a href='http://webring.xxiivv.com/'><img src='media/icon/rotonde.svg' /></a>\n", f);
	fputs("<a href='https://lieu.cblgh.org/'><img src='media/icon/lieu.svg' /></a>\n", f);*/
	fputs("<p>Dave VanEe &copy; 2023 <a href='license.html'>CC-BY-NC-SA 4.0</a></p>\n", f);
	fputs("</section>\n", f);
	fputs("</footer>\n", f);
	fputs("</body>\n</html>\n", f);
	fclose(f);
}

void
fprss(FILE *f, Journal *jou)
{
	int i;
	time_t now;
	fputs("<?xml version='1.0' encoding='UTF-8' ?>\n", f);
	fputs("<rss version='2.0' xmlns:dc='http://purl.org/dc/elements/1.1/'>\n", f);
	fputs("<channel>\n", f);
	fputs("<title>" NAME "</title>\n", f);
	fputs("<link>" DOMAIN "journal.html</link>\n", f);
	fputs("<description>Updates and progress on various projects and day to day events.</description>\n", f);
	/* Date */
	fputs("<lastBuildDate>", f);
	fpRFC2822(f, time(&now), 1);
	fputs("</lastBuildDate>\n", f);
	/* Image */
	fputs("<image>\n", f);
	fputs("  <url>" DOMAIN "media/icon/rss.png</url>\n", f);
	fputs("  <title>Updates and progress on various projects and day to day events.</title>\n", f);
	fputs("  <link>" DOMAIN "/journal.html</link>\n", f);
	fputs("</image>\n", f);
	for(i = 0; i < jou->len; ++i) {
		Log l = jou->logs[i];
		fputs("<item>\n", f);
		fprintf(f, "  <title>%s</title>\n", l.name);
		if(l.term)
			fprintf(f, "  <link>" DOMAIN "%s.html</link>\n", l.term->filename);
		if(l.pict)
			fprintf(f, "  <guid isPermaLink='false'>%s</guid>\n", l.pict);
		fputs("  <pubDate>", f);
		fpRFC2822(f, itotime(sint(l.date, 6)), 1);
		fputs("</pubDate>\n", f);
		fputs("  <dc:creator><![CDATA[tbsp]]></dc:creator>\n", f);
		fputs("  <description>\n", f);
		fputs("<![CDATA[", f);
		if(l.pict)
			fprintf(f, "<figure><a href='" DOMAIN "media/%s.jpg' alt='Click the image for a higher resolution'><img src='" DOMAIN "media/%s-680.png'/></a><figcaption>%s: %s</figcaption></figure>\n", l.pict, l.pict, l.date, l.name);
		if(l.term)
			fprintf(f, "<p>Filed under <i>%s/<a href='" DOMAIN "%s.html'>%s</a>: %s</i></p>", l.term->host, l.term->filename, l.term->filename, l.term->bref ? l.term->bref : "");
		fputs("]]>\n", f);
		fputs("  </description>\n", f);
		fputs("</item>\n", f);
	}
	fputs("</channel>", f);
	fputs("</rss>", f);
	fclose(f);
}

#pragma mark - Parse

int
parse_glossary(FILE *fp, Block *block, Glossary *glo)
{
	int len, depth, count = 0, split = 0;
	char line[512], buf[1024], subs[512];
	List *l = &glo->lists[glo->len];
	while(fgets(line, 512, fp)) {
		depth = spad(line, '\t');
		len = slen(strm(line));
		count++;
		if(len < 2 || line[0] == ';')
			continue;
		if(glo->len >= GLOMEM)
			return errorid("Increase memory", "glossary", glo->len);
		if(len > 400)
			return errorid("Line is too long", line, len);
		if(depth == 0)
			l = makelist(&glo->lists[glo->len++], push(block, sstr(line, buf, 0, len)));
		else if(depth == 1) {
			if(l->len >= ITEMS)
				errorid("Reached list item limit", l->name, l->len);
			split = ssin(line, " : ");
			if(split < 0)
				l->vals[l->len] = push(block, sstr(line, buf, 1, len + 1));
			else {
				l->keys[l->len] = push(block, sstr(line, buf, 1, split - 1));
				if(!surl(line))
					l->vals[l->len] = push(block, sstr(line, buf, split + 2, len - split));
				else {
					scpy(sstr(line, buf, split + 2, len - split), subs, len - split);
					split = ssin(subs, " : ");
					if(split > 0) {
						l->vals[l->len] = push(block, sstr(subs, buf, split + 3, slen(subs) - split));
						l->url[l->len] = push(block, sstr(subs, buf, 0, split));
					} else
						l->url[l->len] = push(block, subs);
				}
			}
			l->len++;
		}
	}
	printf(":%d ", count);
	return 1;
}

int
parse_lexicon(FILE *fp, Block *block, Lexicon *lex)
{
	int key_len, val_len, len, count = 0, catch_body = 0, catch_link = 0;
	char line[1024], buf[1024];
	Term *t = &lex->terms[lex->len];
	while(fgets(line, 1024, fp)) {
		int depth = spad(line, '\t');
		strm(line);
		len = slen(line);
		count++;
		if(len < 3 || line[0] == ';')
			continue;
		if(lex->len >= LEXMEM)
			return errorid("Increase memory", "Lexicon", lex->len);
		if(len > 750)
			return errorid("Line is too long", line, len);
		if(depth == 0) {
			t = maketerm(&lex->terms[lex->len++], push(block, sstr(line, buf, 0, len)));
			if(!sian(line))
				return error("Lexicon key is not alphanumerical", line);
			t->filename = push(block, scsw(slca(sstr(line, buf, 0, len)), ' ', '_'));
		} else if(depth == 1 && len > 2) {
			if(ssin(line, "HOST : ") >= 0)
				t->host = push(block, sstr(line, buf, 8, len - 8));
			if(ssin(line, "BREF : ") >= 0)
				t->bref = push(block, sstr(line, buf, 8, len - 8));
			if(ssin(line, "TYPE : ") >= 0)
				t->type = push(block, sstr(line, buf, 8, len - 8));
			if(ssin(line, "DATE : ") >= 0)
				t->date = push(block, sstr(line, buf, 8, len - 8));
			if(ssin(line, "CAPT : ") >= 0)
				t->caption = push(block, sstr(line, buf, 8, len - 8));
			catch_body = ssin(line, "BODY") >= 0;
			catch_link = ssin(line, "LINK") >= 0;
		} else if(depth == 2 && len > 3) {
			if(catch_body)
				t->body[t->body_len++] = push(block, sstr(line, buf, 2, len - 2));
			else if(catch_link) {
				key_len = scin(line, ':') - 3;
				if(key_len < 0)
					return errorid("Invalid link", line, count);
				t->link.keys[t->link.len] = push(block, sstr(line, buf, 2, key_len));
				val_len = len - key_len - 3;
				t->link.vals[t->link.len++] = push(block, sstr(line, buf, key_len + 5, val_len));
			} else
				return errorid("Invalid line", line, count);
		} else
			return errorid("Invalid line", line, count);
	}
	printf(":%d ", count);
	return 1;
}

int
parse_journal(FILE *fp, Block *block, Lexicon *lex, Journal *jou)
{
	int len, count = 0;
	char line[1024], buf[1024], path[128];
	FILE *img;
	Log *l = &jou->logs[jou->len];
	while(fgets(line, 1024, fp)) {
		len = slen(strm(line));
		count++;
		if(len < 14 || line[0] == ';')
			continue;
		if(jou->len >= LOGMEM)
			return errorid("Increase memory", "Journal", jou->len);
		if(len > 1024)
			return errorid("Log is too long", line, len);
		l = makelog(&jou->logs[jou->len++], push(block, strm(sstr(line, buf, 0, 6))));
		l->term = findterm(lex, strm(sstr(line, buf, 7, JORNAL_HOST_LENGTH)));
		path[0] = '\0';
		scat(path, "journal/");
		scat(path, l->date);
		img = getfile("../media/", path, ".jpg", "r");
		if(img) {
			l->pict = push(block, path);
			fclose(img);
		} else if(l->term) {
			path[0] = '\0';
			scat(path, "headers/");
			scat(path, l->term->filename);
			l->pict = push(block, path);
		} else
			return error("Couldn't open image", path);
		if(len >= 29)
			l->name = push(block, strm(sstr(line, buf, 7 + JORNAL_HOST_LENGTH + 1, len)));
	}
	printf(":%d ", count);
	return 1;
}

int
parse(Block *block, Glossary *glo, Lexicon *lex, Journal *jou)
{
	FILE *fglo = fopen("database/glossary.ndtl", "r");
	FILE *flex = fopen("database/lexicon.ndtl", "r");
	FILE *fjou = fopen("database/journal.tbtl", "r");
	printf("Parsing | ");
	printf("glossary");
	if(!fglo || !parse_glossary(fglo, block, glo)) {
		fclose(fglo);
		return error("Parsing", "Glossary");
	}
	printf("lexicon");
	if(!flex || !parse_lexicon(flex, block, lex)) {
		fclose(flex);
		return error("Parsing", "Lexicon");
	}
	printf("journal");
	if(!fjou || !parse_journal(fjou, block, lex, jou)) {
		fclose(fjou);
		return error("Parsing", "Journal");
	}
	fclose(fglo);
	fclose(flex);
	return 1;
}

int
link(Glossary *glo, Lexicon *lex)
{
	int i, j;
	printf("Linking  | ");
	printf("glossary:%d ", glo->len);
	printf("lexicon:%d ", lex->len);
	for(i = 0; i < lex->len; ++i) {
		Term *t = &lex->terms[i];
		for(j = 0; j < t->body_len; ++j)
			fptemplate(NULL, glo, lex, t, t->body[j]);
		t->parent = findterm(lex, t->host);
		if(scmp(t->name, "404"))
			continue;
		if(!t->parent)
			return error("Missing parent", t->host);
		if(!t->bref && !t->type)
			return error("Missing bref", t->name);
		t->parent->children[t->parent->children_len++] = t;
	}
	return 1;
}

int
build(Glossary *glo, Lexicon *lex, Journal *jou)
{
	FILE *f;
	int i;
	printf("Building | ");
	printf("%d pages ", lex->len);
	for(i = 0; i < lex->len; ++i) {
		f = getfile("../", lex->terms[i].filename, ".html", "w");
		if(!f)
			return error("Could not open file", lex->terms[i].name);
		fphtml(f, glo, lex, &lex->terms[i], jou);
	}
	f = fopen("../links/rss.xml", "w");
	if(!f)
		return error("Could not open file", "rss.xml");
	fprss(f, jou);
	return 1;
}

void
check_lists(Glossary *glo)
{
	int i;
	printf("Checking Lists | ");
	for(i = 0; i < glo->len; i++) {
		List *l = &glo->lists[i];
		if(l->routes < 1)
			printf("Warning: Unused list \"%s\"\n", l->name);
	}
}

Block block;
Glossary all_lists;
Lexicon all_terms;
Journal all_logs;
clock_t start;

int
main(void)
{
	printf("Building %s...\n", NAME);

	start = clock();
	if(!parse(&block, &all_lists, &all_terms, &all_logs))
		return error("Failure", "Parsing");
	printf("[%.2fms]\n", clockoffset(start));

	start = clock();
	if(!link(&all_lists, &all_terms))
		return error("Failure", "Linking");
	printf("[%.2fms]\n", clockoffset(start));

	start = clock();
	if(!build(&all_lists, &all_terms, &all_logs))
		return error("Failure", "Building");
	printf("[%.2fms]\n", clockoffset(start));

	start = clock();
	check_lists(&all_lists);
	printf("[%.2fms]\n", clockoffset(start));

	printf("%d/%d characters in memory\n", block.len, STRMEM);
	return 0;
}
