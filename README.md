### Cfs
В основу этой библиотеки легла идея о JSON-pure API, которое не взирает на коды http, а оперирует исключительно с JSON.
Планируется сделать:  
#### 1. Первый слой абстракции:
  * Свой сервер на C++ -- готово
#### 2. Второй слой абстракции: 
  * Надстройку над сервером оперирующую входящими и исходящими JSON-файлами. 
  * Это позволит пользователю библиотеки абстрагироваться от работы сервера и сосредоточиться на разработке своего собственного API.
#### 3. Третий слой абстракции:
  * Передавать любые классы данных клиенту, не думая в каком формате они передаются.
  * Сохранение классов данных осуществляется автоматически в локальную NoSQL базу данных.
  * Это позволит думать и вести разработку в парадигме Domain-driven Design.

