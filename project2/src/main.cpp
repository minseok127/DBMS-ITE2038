#include <iostream>
#include "../include/bpt.h"

using namespace std;

int main(int argc, char* argv[]){
    char instruction;
    int64_t input;
    int table_id;
    char value[120];

    char* path;
    scanf("%s", path);
    getchar();
    table_id = open_table(path);

    cout << "table id: " << table_id << endl;
    cout << "> ";
    while(1){
        cin >> instruction;
        switch (instruction)
        {
        case 'i':
            cout << "key: ";
            cin >> input;
            cout << "value: ";
            cin >> value;
            db_insert(input, value);
            break;
        case 'd':
            cout << "key: ";
            cin >> input;
            db_delete(input);
            break;
        case 't':
            print_tree();
            break;
        case 'f':
            cout << "find key: ";
            cin >> input;
            db_find(input, value);
            cout << "value: " << value << endl;
            break;
        case 's':
            scan();
            break;
        default:
            break;
        }
        while(getchar() != (int)'\n');
        cout << "> ";
    }
    return 0;
}