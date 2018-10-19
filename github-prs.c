/* github-prs.c */

#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "json-c/json.h"
#include <locale.h>
#include <unistd.h>
#include <signal.h>

#define ITERMAX 10000

void sigint_handler(int sig_num)
{
    /* Reset handler to catch SIGINT next time.
       Refer http://en.cppreference.com/w/c/program/signal */
    printf("\n User provided signal handler for Ctrl+C \n");

    endwin();
    /* Do a graceful cleanup of the program like: free memory/resources/etc and exit */
    exit(0);
}

void removeChar(char *str, char *garbage) {

    char *src, *dst;
    for (src = dst = str; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != *garbage) dst++;
    }
    *dst = '\0';
}

char * getValue(struct json_object *object, char *key) {
    struct json_object *value;
    char * stringval;
    value = json_object_object_get(object, key);
    stringval = json_object_to_json_string_ext(value, JSON_C_TO_STRING_PRETTY);
    return stringval;
}

void queryApi(char * command, int row, int col) {
    const int padding = 4;
    
    FILE *result;
    result = popen(command, "r");
    
    char * buffer = NULL;
    struct json_object *jobj;
    struct json_object *object;
    size_t len;
    ssize_t bytes_read = getdelim(&buffer, &len, '\0', result);
    if (bytes_read != -1) {
        jobj = json_tokener_parse(buffer);
        for (int i = 0; i < 5; i++) {
            object = json_object_array_get_idx(jobj, i);
            if (object != NULL) {
                char *title = getValue(object, "title");
                removeChar(title, "\\");
                removeChar(title, "\"");

                char *url = getValue(object, "html_url");
                removeChar(url, "\\");
                removeChar(url, "\"");

                char *updated_at = getValue(object, "updated_at");
                removeChar(updated_at, "\\");
                removeChar(updated_at, "\"");

                unsigned long left = (col-strlen(url))/2;
                unsigned long leftpad = left > (col - 80) / 2 ? left : (col - 80) / 2;

                attron(COLOR_PAIR(1));
                mvprintw(padding+5+(i * 5), leftpad,"%s",title);
                attroff(COLOR_PAIR(1));

                attron(COLOR_PAIR(2));
                mvprintw(padding+6+(i * 5), leftpad,"%s",url);
                attroff(COLOR_PAIR(2));

                attron(COLOR_PAIR(3));
                mvprintw(padding+7+(i * 5), leftpad,"Updated: %s",updated_at);
                attroff(COLOR_PAIR(3));
            }
        }
    }

}

void drawBorder(int row, int col) {
    for (int column = 0; column < col; column++) {
            mvprintw(0, column,"%s", "=");
            mvprintw(row-1, column,"%s", "=");
    }
    for (int vertical = 0; vertical < row; vertical++) {
            mvprintw(vertical, 0, "%s", "|");
            mvprintw(vertical, col-1, "%s", "|");
    }
}

int printHelp() {
    printf("%s", "GITHUB PRS\n");
    printf("%s", "-------------------------\n");
    printf("%s", "Add the following to your env / .bashrc / .zshrc:\n");
    printf("%s", "export GITHUB_USERNAME=<your github username, ex \"xp-bar\">\n");
    printf("%s", "export GITHUB_REPO=<the github repo, including owner you want to use; ex. \"xp-bar/repo\">\n");
    printf("%s", "export GITHUB_TOKEN=<your github token, make one here: https://github.com/settings/tokens/new>\n");
    printf("%s", "-------------------------\n");
    printf("%s", "Also, ensure your git config user.email is set to one associated with your token: git config user.name \"your.email@yourdomain.ca\"\n");
    printf("%s", "-------------------------\n");
    printf("%s", "Then, finally, you can run with: ./prs assigned, ./prs -a; ./prs created, ./prs -c\n");
    return 1;
}

int main(int argc, char **argv)
{
    signal(SIGINT, sigint_handler);

    /* ENv VARIABLES */
    if (argc == 1 || strncmp(argv[1], "--help", 7) == 0) {
        return printHelp();
    }

    int row,col;
    const int MAX_SIZE = 1024;
    const int padding = 4;
    
    const char* baseurl="https://api.github.com/repos/";
    char * repo = getenv("GITHUB_REPO");
    char* user = getenv("GITHUB_USERNAME");
    char* token = getenv("GITHUB_TOKEN");
    char * emailstr = NULL;

    char * whichType = "";
    char * whichTypeString = "";

    if (argc > 1) {
        for(int i=1; i < argc; i++) {
            char * id = *(argv+i);
            int nextIndex = i+1;
            char * next = *(argv+nextIndex);

            // USERNAME
            if (strcmp(id, "-u") == 0) {
                if (next != NULL) {
                    user = next;
                } else {
                    printf("You forgot a username after -u!");
                    exit(1);
                }
            }

            // REPO
            if (strcmp(id, "-r") == 0) {
                if (next != NULL) {
                    char tmp[256];
                    snprintf(tmp, sizeof tmp, "%s/", next);
                    repo = tmp;
                } else {
                    printf("You forgot a owner/repo after -r!");
                    exit(1);
                }
            }

            // TOKEN
            if (strcmp(id, "-t") == 0 && next != NULL) {
                if (next != NULL) {
                    token = next;
                } else {
                    printf("You forgot a token after -t!");
                    exit(1);
                }
            }

            // EMAIL
            if (strcmp(id, "-e") == 0 && next != NULL) {
                if (next != NULL) {
                    emailstr = next;
                } else {
                    printf("You forgot an email after -e!");
                    exit(1);
                }
            }

            if (strcmp(id, "-c") == 0 || strcmp(id, "--created") == 0) {
                whichType = "creator";
                whichTypeString = "CREATED BY";
            }

            if (strcmp(id, "-a") == 0 || strcmp(id, "--assigned") == 0) {
                whichType = "assignee";
                whichTypeString = "ASSIGNED TO";
            }
        }
    }

    if (emailstr == NULL) {
        char estr[256];
        char buff[1024];
        FILE* email = popen("git config user.email", "r");
        while (fgets(buff, MAX_SIZE, email) != NULL) {
            buff[strcspn(buff, "\n")] = 0;
            snprintf(estr, sizeof estr, "%s", buff);
        }
        emailstr = estr;
    }

    if (repo == NULL) {
        printf("%s", "Make sure you either pass a repo (-r owner/repo) or set your GITHUB_REPO env variable: GITHUB_REPO=\"owner/myrepo\"");
        return 1;
    }

    if (user == NULL) {
        printf("%s", "Make sure you set your GITHUB_USERNAME env variable: GITHUB_USERNAME=xp-bar");
        return 1;
    }

    if (token == NULL) {
        printf("%s", "Make sure you set your GITHUB_TOKEN env variable with your github api personal token: GITHUB_TOKEN=\"<token>\"");
        return 1;
    }


    if (strcmp(whichType, "") == 0) {
        whichType = "creator";
        whichTypeString = "CREATED BY";
    }

    char url[256];
    char login[256];

    snprintf(url, sizeof url, "%s%s%s%s%s%s", baseurl, repo, "issues?", whichType, "=", user);
    snprintf(login, sizeof login, "%s%s%s", emailstr, ":", token);
    
    initscr();
    start_color(); 
    use_default_colors();
    noecho();
    curs_set(0);
    getmaxyx(stdscr,row,col);
    if (col < 80) {
        endwin();
        printf("%s", "Your screen is too small! Try making it bigger and trying again.");
        exit(1);
    }
	init_pair(1, COLOR_RED, -1);
	init_pair(2, COLOR_GREEN, -1);
	init_pair(3, COLOR_BLUE, -1);

    char userstring[256];
    snprintf(userstring, sizeof userstring, "%s%s%s%s%s", "PULL REQUESTS ", whichTypeString, " ", user, ":");
    attron(A_BOLD);
    mvprintw(padding+1,(col-40)/2,"%s","----------------------------------------");
    mvprintw(padding+2,(col-strlen(userstring))/2,"%s",userstring);
    mvprintw(padding+3,(col-40)/2,"%s","----------------------------------------");
    attroff(A_BOLD);

    char command[256];
    snprintf(command, sizeof command, "%s%s%s%s", "curl -s -u ", login, " ", url);

    char chr;
    timeout(0);
    do {
        queryApi(command, row, col);
        drawBorder(row, col);
        mvprintw(row-2,2,"%s", "Press ctrl+c to quit.");
        refresh();
        sleep(30);
    } while ((chr = getch()) == ERR);

    return 0;
}

