package it.cnr.coco.utils;

public class Assertion {

    /**
     * Throws an AssertionError if the condition is false.
     *
     * @param condition the condition to check
     * @throws AssertionError if the condition is false
     */
    public static void assertCondition(boolean condition) {
        if (!condition)
            throw new AssertionError("Assertion failed");
    }

    /**
     * Throws an AssertionError with a custom message if the condition is false.
     *
     * @param condition the condition to check
     * @param message   the message to include in the AssertionError
     * @throws AssertionError if the condition is false
     */
    public static void assertCondition(boolean condition, String message) {
        if (!condition)
            throw new AssertionError("Assertion failed: " + message);
    }
}
