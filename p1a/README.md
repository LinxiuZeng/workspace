# Code Description
Code Structure: Singly Linked List 
>Linked List Node includes three parts: int key, char* value, and pointer to the next linked node in the list 

Included methods: put, get, all, delete, clear, read_file, main 

>put (int key, char* value): check whether the inserted value is a valid character + handle duplicate. Then insert the key-value pair as the front of the linked list. 

>get (int target): with return value the specific node with correct key value. Starting from head, traversly search through all the linkedlist until either find the target key, or, if reach the end of the list, return NULL. 

>all ():  Starting from head, traversly print all the linkedlist nodes. 

>delete(int target): similar to put, first find the node then change the next pointer of the previous node to the next node

>clear(): clear all elements within linkedlist 

>read_file ()： check whether the assigned file can be correctly open. 

>main: imported all the existed key-value pair stored in the txt file + handle commands + store linkedlist elements into a txt file.



