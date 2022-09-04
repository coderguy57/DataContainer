# Multithreading

## Perspective

The general perspective towards multithreading that this section is written towards is the following: Accessing data is "safe" if it won't crash the program and any value you read is either the value resulting from default construction or a value that was assigned to a value of the same logical type (i.e. to that same property) at some other point. For example, you will never get a `NaN` from a floating point value unless you have written a `NaN` into some floating point value of that type at some point in time. These values are not, however, guaranteed to be correct, so you should also perform validation if you read values in a context where another thread might be writing them.

Safe in this context does *not* mean free of undefined behavior. Reading while writing may involve data races. Safe only means that I expect it to work on the target platform (x86_64).

## Safe operations

If an object or relationship is *not* defined as having an `expandable` size, then any operation that reads data from the data container is designed to be safe, as described above. This includes operations that may require reading multiple values internally, such as iterating over all of the relationship instances linked to an object instance. You may not, however, assume that the results of such operations are correct or logically consistent. Concurrent modifications could easily result in not seeing some relationships during such an iteration (even if they are not removed by any concurrent modifications) or seeing a relationship instance more than once. This safety extends to reading the contents of a property of the `vector_pool` type. It does not, however, extend to properties with type `object` unless those objects make their own compatible safety guarantees. An exception to this rule is looking up relationship instances with a composite key; doing so is not safe if any relationships of that type may be concurrently added or removed. 

Assuming I haven't overlooked something, all of the functions that can be accessed through a `const` reference or pointer to a data container will be safe in this way (with the exception of accessing `object` type properties, for which getters are provided because access *might* be safe, but only the user can know for sure).

## Probably bad ideas

Trying to write values to the same location from two different threads at the same time is probably a bad idea, for all of the normal reasons. Trying to change the object instances linked in two different relationship instances but in the same type of link is possible *if* they are not involved in a composite key and if neither any of the new object instances nor old object instances overlap. Since there are so many ways this can go wrong, you are strongly advised not to attempt it. Modifying two different `vector_pool` values that are fundamentally backed by the same memory pool *should* be safe (the memory pool uses lock-free techniques to make modification safe), but since it is so easy to make mistakes with implementing such things correctly, I can only say that I *hope* it works properly. And, as already mentioned, looking up relationship instances with a composite key requires knowing that no other thread is adding or removing relationships of that type.

## Definitely bad ideas

Adding or deleting object instances or relationship instances from two or more threads at the same time is definitely a bad idea, that will almost certainly leave the data container in an inconsistent state. Accessing any data for properties/links in a relationship or object instance with size `expandable` while an instance may be created in another thread is also forbidden. Finally, accessing data from a property with type `array` while that array is being resized from another thread is also an error. Since deserializing information into a data container can do any or all of the things mentioned above, it is probably best to prevent even concurrent reads while deserialization is occurring[^1].

## Expected usage pattern

The data container is designed for the following multithreading usage pattern: any definitely unsafe operations (such as resizing type `array` properties) is done in a special, single-threaded initialization phase. Subsequently there may be any number of threads reading data from it (ideally through `const` references/pointers), while the "probably bad ideas" and all creation/deletion of object/relationship instances will all be confined to a single thread. Modifying multiple values for the same logical property will only be done from multiple threads simultaneously when the threads can be given disjoint ranges of object/relationship instances to modify to ensure that they make no attempt to write to the same values at the same time. (This pattern, of course, means that no objects or relationships can have size `expandable`)

[^1]: Doing that may be one of the few situations in which a shared pointer works well. Access to the data container can be gated though making a copy of the shared pointer (ideally once for a substantial usage of that pointer, not once per accessing every function). Then data can be deserialized into a new instance of the data container which can simply replace the old one in the shared pointer after deserialization is complete.