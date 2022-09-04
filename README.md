# DataContainer

## What is it?

The DataContainer project contains a code generation tool that turns text specifications describing objects and their relationships into a C++ header files defining a `data_container` class that manages the described objects and relationships.

## Why would anyone want to use this?

Since that sounds a lot like what `struct`s and `class`es in a C++ header file already are, why would anyone want to go through the extra trouble required to use a code generator? The generated data container is intended to solve the following problems:

- Values stored in standard C++ objects end up arranged in memory in a way that is relatively unfriendly to SIMD operations. The standard C++ solution is to use a "struct of arrays" rather than an "array of structs", but actually doing this can be quite cumbersome. The generated data container, however, automatically stores values for the same property in contiguous memory and exposes a user-friendly SIMD interface to them.
- Managing relationships between objects involves both repetitive boilerplate and is a common source of bugs. The code required to manage the relationships is also often hard to make changes to without significant effort. The generated data container helps solve this problem by automatically generating the code required to keep relationships up to date as objects are created and deleted, and automatically maintains any indexes you require for quickly finding an object based on a given relationship, or vice versa.
- Serializing and deserializing C++ objects is a pain in the neck. It is easy enough to dump their contents out to a file, but anything non trivial requires more than that. You may want the ability to selectively save some of an object's properties and not others when it is possible to recalculate that information without saving it directly. And you almost certainly want the ability to load serialized data created by old versions of your software, even after member variables have been added, removed, or changed type. Because the generator is provided a description of your objects and relationships that it can understand as a whole, it can provide you with robust serialization and deserialization routines.
- And yes, there are existing libraries for doing all of the above things (often quite good ones). However, the problems discussed above are all fundamentally about storing and managing the data that is structured into objects and relationships. Thus, using a library to solve one of these problems may very well make it harder to use a library to solve another of them. And so the approach of this project is to try to solve them all in one go, and thus to be able to hide the sometimes ugly details from the end user completely.
- OK, but why a code generator? Some early iterations of these ideas were implemented with dark template magics, but doing so made compilation significantly slower. So while it is harder to initially integrate a code generator into your project, I think that ultimately it is the more user friendly solution.

## Documentation

The following documentation explains all you need to know about how to generate and use data containers.

- [Getting started](getting_started.md)
- [Overview](overview.md)
- [Objects and properties](objects_and_properties.md)
- [Relationships](relationships.md)
- [Serialization and deserialization](serialization.md)
- [Multithreading](multithreading.md)
- [The ve SIMD library](ve_documentation.md)
- [File format](file_format_documentation.md)

## Feedback

Feel free to leave comments and suggestions as you see fit. If you are actively using, or intend to use, a data container generated by this project in one of your own projects, I will try to prioritize any fixes or features that you need. Otherwise, additions will be mostly driven by my own needs.