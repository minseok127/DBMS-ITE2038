#include <iostream>
#include "../include/bpt.h"

using namespace std;

int main(int argc, char* argv[]){
    char instruction;
    int64_t input;
    int table_id;
    char value[120];
    int num_buf;
    cout << "input number of buf : ";
    cin >> num_buf;
    init_db(num_buf);

    char* path = new char[100];
    cout << "input path: ";
    cin >> path;
    table_id = open_table(path);
    cout << "you open table number : " << table_id << endl;

    cout << "> ";
    while(1){
        cout << "choose table id and instruction: ";
        scanf("%d %c", &table_id, &instruction);
        switch (instruction)
        {
        case 'i':
            cout << "key: ";
            cin >> input;
            cout << "value: ";
            cin >> value;
            db_insert(table_id, input, value);
            break;
        case 'd':
            cout << "key: ";
            cin >> input;
            db_delete(table_id, input);
            break;
        case 't':
            print_tree(table_id);
            break;
        case 'f':
            cout << "find key: ";
            cin >> input;
            if(db_find(table_id, input, value) == -1){
                cout << "no value" << endl;
                break;
            }
            cout << "value: " << value << endl;
            break;
        case 's':
            scan(table_id);
            break;
        case 'b':
            print_buf(num_buf);
            break;
        case 'p':
            cout << "input path: ";
            cin >> path;
            table_id = open_table(path);
            cout << "you open table number : " << table_id << endl;
            break;
        case 'c':
            if (close_table(table_id) == -1){
                cout << "error" << endl;
            }
            break;
        case 'v':
            cout << shutdown_db() << endl;
            break;
        default:
            break;
        }
        while(getchar() != (int)'\n');
        cout << "> ";
    }
    return 0;
}