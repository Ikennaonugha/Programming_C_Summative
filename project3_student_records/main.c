#include "student.h"

/* One menu entry: the label shown to the user and the function it runs. */
typedef struct {
    const char   *label;
    OperationFunc func;
} MenuItem;

int main(void) {
    if (!init_registry())
        return EXIT_FAILURE;

    /* Jump table: menu choices map to functions through function pointers. */
    const MenuItem menu[] = {
        {"Register a new student",       register_student},
        {"Display all students",         display_students},
        {"View student details",         view_student},
        {"Update student information",   update_student},
        {"Delete a student record",      delete_student},
        {"Add a course to a student",    add_course},
        {"Update marks for a course",    update_marks},
        {"Display student performance",  display_student_performance},
        {"Search for students",          search_students},
        {"Sort student records",         sort_students},
        {"Generate performance report",  generate_report},
        {"Export report to a text file", export_report},
        {"Exit program",                 exit_program}
    };
    const size_t menu_size = sizeof menu / sizeof menu[0];

    char pause_buf[8];
    for (;;) {
        print_header();
        printf("\n--- MAIN MENU ---\n");
        for (size_t i = 0; i < menu_size; i++)
            printf("[%zu] %s\n", i + 1, menu[i].label);
        printf("-------------------------------------------------------------------\n");

        int choice;
        if (!read_int("Enter your choice: ", 1, (int)menu_size, &choice))
            exit_program(); /* input closed: clean up and quit */

        menu[choice - 1].func(); /* dispatch through the function pointer */

        read_line("\nPress Enter to continue...", pause_buf, sizeof pause_buf);
    }
}
